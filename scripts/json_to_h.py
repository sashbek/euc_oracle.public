#!/usr/bin/env python3
"""
Конвертер registers.json -> settings_register_map.h
Генерирует:
- RegAddr enum (адреса регистров)
- SettingIndex enum (индексы настроек)
- SETTINGS_REGISTER_MAP макрос
- SETTINGS_MAPPING макрос (сопоставление индексов и адресов)
"""

import json
import sys
from typing import Dict, Any, List, Optional
from collections import OrderedDict

# ============ Вспомогательные функции ============

def format_default_value(reg: Dict) -> str:
    """Форматирует значение по умолчанию в строку C."""
    if 'defaultValue' not in reg:
        return 'nullptr'
    
    val = reg['defaultValue']
    reg_type = reg.get('type', 'UINT8')
    
    # Булево значение
    if isinstance(val, bool):
        return '(const uint8_t*)"\\x01"' if val else '(const uint8_t*)"\\x00"'

    if reg_type == 'BOOLEAN':
        if isinstance(val, str):
            v = val.lower() == 'true'
        else:
            v = bool(val)
        return '(const uint8_t*)"\\x01"' if v else '(const uint8_t*)"\\x00"'
    elif reg_type in ('UINT8', 'INT8', 'ENUM'):
        return f'(const uint8_t*)"\\x{int(val):02X}"'
    elif reg_type in ('UINT16', 'INT16'):
        return f'(const uint8_t*)"\\x{int(val) & 0xFF:02X}\\x{(int(val) >> 8) & 0xFF:02X}"'
    elif reg_type in ('UINT32', 'INT32'):
        v = int(val)
        return (f'(const uint8_t*)"\\x{v & 0xFF:02X}\\x{(v >> 8) & 0xFF:02X}'
                f'\\x{(v >> 16) & 0xFF:02X}\\x{(v >> 24) & 0xFF:02X}"')
    elif reg_type == 'COLOR_RGBW':
        # Формат "#RRGGBBWW" или "#RRGGBB"
        if isinstance(val, str) and val.startswith('#'):
            hex_str = val.lstrip('#')
            if len(hex_str) == 6:  # RGB -> RGBW
                hex_str += '00'
            elif len(hex_str) == 8:  # RGBW
                pass
            else:
                return 'nullptr'
            
            # Парсим компоненты: JSON хранит RRGGBBWW, в коде порядок WWBBGGRR (little-endian)
            try:
                r = int(hex_str[0:2], 16)
                g = int(hex_str[2:4], 16)
                b = int(hex_str[4:6], 16)
                w = int(hex_str[6:8], 16)
            except ValueError:
                return 'nullptr'
            
            # Порядок байт в памяти: R, G, B, W
            return (f'(const uint8_t*)"\\x{r:02X}\\x{g:02X}\\x{b:02X}\\x{w:02X}"')
        else:
            v = int(val)
            return (f'(const uint8_t*)"\\x{v & 0xFF:02X}\\x{(v >> 8) & 0xFF:02X}'
                    f'\\x{(v >> 16) & 0xFF:02X}\\x{(v >> 24) & 0xFF:02X}"')
    elif reg_type == 'STRING':
        escaped = str(val).replace('\\', '\\\\').replace('"', '\\"')
        return f'(const uint8_t*)"{escaped}"'
    elif reg_type == 'MAC':
        parts = str(val).split(':')
        hex_str = ''.join(f'\\x{int(p, 16):02X}' for p in parts[:6])
        return f'(const uint8_t*)"{hex_str}"'
    else:
        return 'nullptr'

def format_comment(reg: Dict) -> str:
    """Форматирует комментарий с @тегами."""
    lines = [f"/* @name {reg.get('name', 'UNKNOWN')}"]
    
    lines.append(f"   @type {reg.get('type', 'UINT8')}")
    
    if not reg.get('readable', True):
        lines.append(f"   @readable false")
    if not reg.get('writable', False):
        lines.append(f"   @writable false")
    if reg.get('hidden', False):
        lines.append(f"   @hidden true")
    if 'group' in reg:
        lines.append(f"   @group {reg['group']}")
    if 'unit' in reg:
        lines.append(f"   @unit {reg['unit']}")
    if 'scale' in reg:
        lines.append(f"   @scale {reg['scale']}")
    if 'min' in reg:
        lines.append(f"   @min {reg['min']}")
    if 'max' in reg:
        lines.append(f"   @max {reg['max']}")
    if reg.get('bitfield', False):
        lines.append(f"   @bitfield true")
    if 'defaultValue' in reg:
        val = reg['defaultValue']
        if isinstance(val, bool):
            lines.append(f"   @default {'True' if val else 'False'}")
        else:
            lines.append(f"   @default {val}")
    if 'enumValues' in reg:
        parts = [f"{k}:{v}" for k, v in reg['enumValues'].items()]
        lines.append(f"   @enum {','.join(parts)}")
    if 'description' in reg:
        desc = reg['description'].rstrip(' \\')
        lines.append(f"   @desc {desc} \\")
    
    lines.append(" */")
    return '\n'.join(lines)

def group_registers(registers: List[Dict], groups_data: List[Dict]) -> OrderedDict:
    """Группирует регистры по группам из JSON."""
    groups = OrderedDict()
    
    # Создаём маппинг адресов к группам
    addr_to_group = {}
    
    # Сначала определяем группы по полю group в регистрах
    for reg in registers:
        group_name = reg.get('group', 'Other')
        if group_name not in groups:
            groups[group_name] = []
        groups[group_name].append(reg)
    
    # Сортируем регистры внутри групп по адресу
    for group_name in groups:
        groups[group_name].sort(key=lambda r: r['address'])
    
    return groups

def generate_reg_addr_enum(registers: List[Dict], groups_data: List[Dict]) -> str:
    """Генерирует enum RegAddr."""
    groups = group_registers(registers, groups_data)
    
    lines = [
        "// ============================================================================",
        "// АДРЕСА РЕГИСТРОВ - единый enum для всех регистров",
        "// ============================================================================",
        "enum class RegAddr : uint16_t {",
    ]
    
    for group_name, regs in groups.items():
        if not regs:
            continue
        
        # Вычисляем диапазон адресов
        addrs = [r['address'] for r in regs]
        min_addr = min(addrs)
        max_addr = max(addrs)
        
        # Находим описание группы
        desc = group_name
        for gd in groups_data:
            if gd.get('name', '').strip('"') == group_name:
                desc = gd.get('description', group_name)
                break
        
        lines.append(f"    // {group_name} (0x{min_addr:02X} - 0x{max_addr:02X})")
        
        for reg in regs:
            name = reg['name'].upper()
            addr = reg['address']
            lines.append(f"    {name:<35} = 0x{addr:02X},")
        
        lines.append("")
    
    lines.append("};")
    return '\n'.join(lines)

def generate_setting_index_enum(registers: List[Dict], groups_data: List[Dict]) -> str:
    """Генерирует enum SettingIndex только для персистентных регистров."""
    persistent = [r for r in registers if r.get('persistent', False)]
    groups = group_registers(persistent, groups_data)
    
    lines = [
        "// ============================================================================",
        "// Индексы настроек (только для персистентных регистров)",
        "// ============================================================================",
        "enum class SettingIndex : uint16_t {",
    ]
    
    # Назначаем диапазоны индексов для групп (фиксированные смещения)
    group_index_offsets = {
        "ThirdEye":   0x00,
        "Extentions": 0x20,
        "LED Effects":0x40,
        "UART":       0x60,
        "System":     0x80,
    }
    
    for group_name, regs in groups.items():
        if not regs:
            continue
        
        base_idx = group_index_offsets.get(group_name)
        if base_idx is None:
            # Автоматическое смещение для неизвестных групп
            base_idx = 0xC0
        
        lines.append(f"    // {group_name}")
        for i, reg in enumerate(regs):
            name = reg['name'].upper()
            idx = base_idx + i
            lines.append(f"    {name:<35} = 0x{idx:02X},")
        lines.append("")
    
    lines.append("    MAX_INDEX")
    lines.append("};")
    return '\n'.join(lines)

def generate_register_map(registers: List[Dict]) -> str:
    """Генерирует макрос SETTINGS_REGISTER_MAP."""
    lines = [
        "// ============================================================================",
        f"// Карта регистров",
        "// Формат: REG(addr, size, readable, writable, persistent, default_value)",
        "// ============================================================================",
        "",
        "#define SETTINGS_REGISTER_MAP \\",
    ]
    
    for reg in registers:
        addr = reg['address']
        size = reg['size']
        readable = 'true' if reg.get('readable', True) else 'false'
        writable = 'true' if reg.get('writable', False) else 'false'
        persistent = 'true' if reg.get('persistent', False) else 'false'
        default_val = format_default_value(reg)
        
        lines.append(f"    {format_comment(reg)} \\")
        lines.append(f"    REG(0x{addr:02X}, {size}, {readable}, {writable}, {persistent}, {default_val}) \\")
        lines.append("     \\")
    
    # Убираем последние пустые строки
    while lines and (lines[-1].strip() in ('', '\\', '     \\')):
        lines.pop()
    
    return '\n'.join(lines)

def generate_settings_mapping(registers: List[Dict]) -> str:
    """Генерирует макрос SETTINGS_MAPPING."""
    persistent = [r for r in registers if r.get('persistent', False)]
    
    lines = [
        "",
        "#define SETTINGS_MAPPING \\",
    ]
    
    for reg in sorted(persistent, key=lambda r: r['address']):
        name = reg['name'].upper()
        addr = reg['address']
        lines.append(f"    index_map_[MonoWheelRegs::SettingIndex::{name}] = 0x{addr:02X};\\")
    
    # Убираем последний \
    if lines:
        last = lines[-1].rstrip('\\')
        if last.strip():
            lines[-1] = last
    
    return '\n'.join(lines)

def generate_h_file(data: Dict) -> str:
    """Генерирует полное содержимое .h файла."""
    registers = data['registers']
    groups_data = data.get('groups', [])
    device_type = data.get('deviceType', 'UNKNOWN')
    
    lines = [
        "#pragma once",
        "",
        "namespace MonoWheelRegs {",
        "",
        generate_reg_addr_enum(registers, groups_data),
        "",
        generate_setting_index_enum(registers, groups_data),
        "",
        "}",
        "",
        generate_register_map(registers),
        "",
        generate_settings_mapping(registers),
        "",
    ]
    
    return '\n'.join(lines)

def main():
    if len(sys.argv) < 3:
        print("Usage: python json_to_h.py <input.json> <output.h>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    with open(input_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    h_content = generate_h_file(data)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(h_content)
    
    print(f"Generated {output_file}")

if __name__ == '__main__':
    main()