package main

import (
	"bufio"
	"fmt"
	"os/exec"
	"strings"
)

type PackageMappings map[int]string

var uidPackageMap = make(PackageMappings)

func LoadPackageMappings() error {
	// 执行系统命令
	cmd := exec.Command("pm", "list", "packages", "-U")
	output, err := cmd.Output()
	if err != nil {
		return fmt.Errorf("failed to execute command: %w", err)
	}

	// 解析命令输出
	scanner := bufio.NewScanner(strings.NewReader(string(output)))
	for scanner.Scan() {
		line := scanner.Text()
		// 样例输出：package:com.example.app uid:10086
		parts := strings.Split(line, " ")
		if len(parts) != 2 {
			continue
		}
		packagePart := strings.TrimPrefix(parts[0], "package:")
		uidPart := strings.TrimPrefix(parts[1], "uid:")
		packageName := strings.TrimSpace(packagePart)
		var uid int
		if _, err := fmt.Sscan(uidPart, &uid); err != nil {
			return fmt.Errorf("failed to parse UID: %w", err)
		}
		uidPackageMap[uid] = packageName
	}

	if err := scanner.Err(); err != nil {
		return fmt.Errorf("error reading command output: %w", err)
	}

	return nil
}

func GetPackageNameByUid(uid int) (string, error) {
	if packageName, ok := uidPackageMap[uid]; ok {
		return packageName, nil
	}
	return "", fmt.Errorf("package name not found for UID %d", uid)
}

