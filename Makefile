GO_RUN = go run github.com/cilium/ebpf/cmd/bpf2go
GO_MOD_TIDY = go mod tidy
GO_BUILD = go build
TARGET = arm64
PACKAGE = main
TYPE = event
BPF_FILE = binder_transaction.byte.c
HEADERS = ./headers
BPF_OUTPUT = bpf
BINARY_NAME = btrace

all: bpf tidy build

bpf:
	$(GO_RUN) -go-package $(PACKAGE) --target=$(TARGET) -type $(TYPE) $(BPF_OUTPUT) $(BPF_FILE) -- -I$(HEADERS)

tidy:
	$(GO_MOD_TIDY)

build: tidy
	$(GO_BUILD)

clean:
	rm -f $(BPF_OUTPUT)_$(TARGET)_bpfel.go $(BPF_OUTPUT)_$(TARGET)_bpfel.o $(BINARY_NAME)

.PHONY: all bpf tidy build clean
