package main

import (
	"bytes"
	"encoding/binary"
	"io"
	"errors"
	"fmt"
)

// ReadUTF16String 从读取器中读取 UTF-16 字符串
func ReadUTF16String(r io.Reader, length uint32) (string, error) {
	buf := make([]byte, length*2) // UTF-16 每个字符占用 2 字节
	if _, err := io.ReadFull(r, buf); err != nil {
		return "", err
	}
	runes := make([]rune, length)
	for i := uint32(0); i < length; i++ {
		runes[i] = rune(binary.LittleEndian.Uint16(buf[i*2:]))
	}
	return string(runes), nil
}

// ExtractInterfaceName 从 Parcel 数据中提取接口名
func ExtractInterfaceName(data []byte) (string, error) {
	reader := bytes.NewReader(data)

	// 判断数据长度是否足够
	if len(data) < 16 {
		return "", errors.New("insufficient data length")
	}

	// 跳过头部的12字节
	if _, err := reader.Seek(12, io.SeekStart); err != nil {
		return "", fmt.Errorf("failed to skip header bytes: %w", err)
	}

	// 读取接口名长度（4 字节）
	var interfaceNameLength uint32
	if err := binary.Read(reader, binary.LittleEndian, &interfaceNameLength); err != nil {
		return "", err
	}

	// 判断剩余数据长度是否足够
	if int(interfaceNameLength)*2 > len(data)-16 {
		return "", errors.New("insufficient data length")
	}	

	// 读取接口名
	interfaceName, err := ReadUTF16String(reader, interfaceNameLength)
	if err != nil {
		return "", err
	}

	return interfaceName, nil
}
