//go:build ignore

#include "common.h"
#include "bpf_tracing.h"
#include "binder_transaction_data.h"

char __license[] SEC("license") = "Dual MIT/GPL";

#define MAX_DATA_LENGTH 0x1000

struct event {
	u32 pid;
	u32 uid;
	u32 code;
	u32 flags;
	u64 data_size;
	u8 data[MAX_DATA_LENGTH];
};

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 1 << 24);
} events SEC(".maps");

// Force emitting struct event into the ELF.
const struct event *unused __attribute__((unused));

SEC("kprobe/binder_transaction")
int kprobe_binder_transaction(struct pt_regs *ctx) {

	int reply = PT_REGS_PARM4(ctx);
	if (reply){
		return 0;
	}

	struct binder_transaction_data *tr = (struct binder_transaction_data *)PT_REGS_PARM3(ctx);
	if (!tr){
		bpf_printk("binder_transaction error: tr is null");
		return 0;
	}

	struct event *binder_transaction_event;
	binder_transaction_event = bpf_ringbuf_reserve(&events, sizeof(struct event), 0);
	if (!binder_transaction_event) {
		bpf_printk("binder_transaction error: event is null");
		return 0;
	}

	binder_transaction_event->pid = bpf_get_current_pid_tgid() >> 32;
	binder_transaction_event->uid = bpf_get_current_uid_gid() >> 32;
	bpf_probe_read(&(binder_transaction_event->code), sizeof(__u32), &(tr->code));
	bpf_probe_read(&(binder_transaction_event->flags), sizeof(__u32), &(tr->flags));
	bpf_probe_read(&(binder_transaction_event->data_size), sizeof(binder_size_t), &(tr->data_size));

	union {
		struct {
			binder_uintptr_t buffer;
			binder_uintptr_t offsets;
		} ptr;
		__u8 buf[8];
	} data;
	bpf_probe_read(&data, sizeof(data), &(tr->data));
	unsigned data_size = binder_transaction_event->data_size;
	if (data_size > MAX_DATA_LENGTH){
		bpf_printk("binder_transaction error: data_size=%d,max_data_size=%d", data_size, MAX_DATA_LENGTH);
	}
	unsigned probe_read_size = data_size < sizeof(binder_transaction_event->data) ? data_size : sizeof(binder_transaction_event->data);
	bpf_probe_read_user(&(binder_transaction_event->data), probe_read_size, (void *)data.ptr.buffer);

	bpf_ringbuf_submit(binder_transaction_event, 0);

	return 0;
}
