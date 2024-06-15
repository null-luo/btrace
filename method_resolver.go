package main

import (
	"encoding/json"
	"fmt"
	_ "embed"
)

//go:embed methods.json
var methodsJSON []byte

type MethodMappings map[string]map[string]string

var interfaceMethods = make(map[string]map[int]string)

// LoadMethodMappings 从嵌入的 JSON 数据加载方法映射
func LoadMethodMappings(jsonData []byte) error {
	var mappings MethodMappings
	if err := json.Unmarshal(jsonData, &mappings); err != nil {
		return fmt.Errorf("failed to unmarshal JSON: %w", err)
	}

	// 转换为 interfaceMethods 的格式
	for interfaceName, methods := range mappings {
		methodMap := make(map[int]string)
		for code, methodName := range methods {
			var codeInt int
			if _, err := fmt.Sscan(code, &codeInt); err != nil {
				return fmt.Errorf("invalid method code: %w", err)
			}
			methodMap[codeInt] = methodName
		}
		interfaceMethods[interfaceName] = methodMap
	}

	return nil
}

// OverrideMethodMappings 用于覆盖默认方法映射
func OverrideMethodMappings(customMethods MethodMappings) {
	for interfaceName, methods := range customMethods {
		if _, ok := interfaceMethods[interfaceName]; !ok {
			interfaceMethods[interfaceName] = make(map[int]string)
		}
		for code, methodName := range methods {
			var codeInt int
			if _, err := fmt.Sscan(code, &codeInt); err == nil {
				interfaceMethods[interfaceName][codeInt] = methodName
			}
		}
	}
}

func GetMethodName(interfaceName string, code int) (string, error) {
	if methods, ok := interfaceMethods[interfaceName]; ok {
		if methodName, ok := methods[code]; ok {
			return methodName, nil
		}
		return "", fmt.Errorf("method not found for code %d in interface %s", code, interfaceName)
	}
	return "", fmt.Errorf("interface %s not found", interfaceName)
}
