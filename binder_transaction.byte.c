//go:build ignore

#include "common.h"
#include "bpf_tracing.h"
#include "binder_transaction_data.h"

char __license[] SEC("license") = "Dual MIT/GPL";

#define CHUNK_SIZE 0x400
#define MAX_CHUNKS 16

struct trace_event {
    u32 pid;
    u32 uid;
    u32 code;
    u32 flags;
    u64 data_size;
    u64 transaction_id;
    u64 chunk_index;
    u8 chunk_data[CHUNK_SIZE];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} trace_event_map SEC(".maps");

struct trace_config {
    u32 uid;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(key_size, sizeof(u32));
    __uint(value_size, sizeof(struct trace_config));
    __uint(max_entries, 1);
} trace_config_map SEC(".maps");

static __always_inline u64 get_transaction_id() {
    u64 id = bpf_ktime_get_ns();
    return id;
}

// Force emitting struct event into the ELF.
const struct trace_event *unused_trace_event __attribute__((unused));
const struct trace_config *unused_trace_config __attribute__((unused));

SEC("kprobe/binder_transaction")
int kprobe_binder_transaction(struct pt_regs *ctx) {

    u32 config_key = 0;
    struct trace_config* conf = bpf_map_lookup_elem(&trace_config_map, &config_key);
    if (conf == NULL) {
        return 0;
    }

    int reply = PT_REGS_PARM4(ctx);
    if (reply) {
        return 0;
    }

    struct binder_transaction_data *tr = (struct binder_transaction_data *)PT_REGS_PARM3(ctx);
    if (!tr) {
        bpf_printk("kprobe_binder_transaction error: tr is null");
        return 0;
    }

    u32 current_uid = bpf_get_current_uid_gid() >> 32;
    if ((conf->uid != 0) && (conf->uid != current_uid)) {
        return 0;
    }

	u32 current_pid = bpf_get_current_pid_tgid() >> 32;
    u64 transaction_id = get_transaction_id();

    u32 code;
    u32 flags;
    u64 data_size;
    bpf_probe_read(&code, sizeof(__u32), &(tr->code));
    bpf_probe_read(&flags, sizeof(__u32), &(tr->flags));
    bpf_probe_read(&data_size, sizeof(binder_size_t), &(tr->data_size));

    union {
        struct {
            binder_uintptr_t buffer;
            binder_uintptr_t offsets;
        } ptr;
        __u8 buf[8];
    } data;
    bpf_probe_read(&data, sizeof(data), &(tr->data));

    u32 total_chunks = (data_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    if (total_chunks > MAX_CHUNKS) {
        bpf_printk("kprobe_binder_transaction error: data size is too long：%d",data_size);
		return 0;
    }

    for (u32 i = 0; i < MAX_CHUNKS; i++) {

		if (i >= total_chunks) {
			return 0;
    	}

        struct trace_event *binder_transaction_event = bpf_ringbuf_reserve(&trace_event_map, sizeof(struct trace_event), 0);
		
        if (!binder_transaction_event) {
            bpf_printk("kprobe_binder_transaction error: failed to reserve ring buffer space");
            return 0;
        }
		
        binder_transaction_event->pid = current_pid;
        binder_transaction_event->uid = current_uid;
        binder_transaction_event->code = code;
        binder_transaction_event->flags = flags;
        binder_transaction_event->data_size = data_size;
        binder_transaction_event->transaction_id = transaction_id;
        binder_transaction_event->chunk_index = i;
		
        u64 chunk_size = ((i + 1) * CHUNK_SIZE > data_size) ? (data_size - i * CHUNK_SIZE) : CHUNK_SIZE;
		unsigned probe_read_size = chunk_size < sizeof(binder_transaction_event->chunk_data) ? chunk_size : sizeof(binder_transaction_event->chunk_data);
        bpf_probe_read_user(binder_transaction_event->chunk_data, probe_read_size, (void *)(data.ptr.buffer + i * CHUNK_SIZE));

		//bpf_printk("kprobe_binder_transaction: transaction_id=%lx,data_size=%d,probe_read_size=%d",transaction_id,data_size,probe_read_size);

        bpf_ringbuf_submit(binder_transaction_event, 0);
    }

    return 0;
}
