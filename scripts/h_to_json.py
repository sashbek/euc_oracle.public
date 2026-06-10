#!/usr/bin/env python3
"""
Конвертер settings_register_map.h -> registers.json
Использование: python h_to_json.py <input.h> <output.json> <device_type>
"""

import re
import json
import sys
from typing import List, Dict, Any, Optional

def parse_default_value(default_str: str, reg_type: str) -> Any:
    """Парсит значение по умолчанию из строки C."""
    if not default_str or default_str == "nullptr":
        return None
    
    # Строка: (const uint8_t*)"\x01" или (const uint8_t*)"MonoWheel"
    match = re.search(r'\(const uint8_t\*\)"((?:\\x[0-9A-Fa-f]{2}|[^"\\])*)"', default_str)
    if match:
        raw = match.group(1)
        # Декодируем \xHH
        def decode_hex(m):
            return chr(int(m.group(1), 16))
        decoded = re.sub(r'\\x([0-9A-Fa-f]{2})', decode_hex, raw)
        
        if reg_type == "BOOLEAN":
            return decoded[0] != '\x00' if decoded else False
        elif reg_type in ("UINT8", "INT8", "ENUM"):
            return ord(decoded[0]) if decoded else 0
        elif reg_type in ("UINT16", "INT16"):
            if len(decoded) >= 2:
                return ord(decoded[0]) | (ord(decoded[1]) << 8)
            return 0
        elif reg_type in ("UINT32", "INT32", "COLOR_RGBW"):
            if len(decoded) >= 4:
                return (ord(decoded[0]) | (ord(decoded[1]) << 8) | 
                       (ord(decoded[2]) << 16) | (ord(decoded[3]) << 24))
            return 0
        elif reg_type == "STRING":
            return decoded.rstrip('\x00')
        elif reg_type == "MAC":
            return ":".join(f"{ord(c):02X}" for c in decoded[:6])
        else:
            return decoded
    return None

def parse_comment(comment: str) -> Dict[str, Any]:
    """Парсит @теги из комментария."""
    tags = {}
    
    # Убираем звездочки и лишние пробелы
    comment = re.sub(r'^\s*\*?\s*', '', comment, flags=re.MULTILINE)
    comment = comment.replace('\n', ' ').replace('\r', '')
    
    # Извлекаем имя
    name_match = re.search(r'@name\s+(\S+)', comment)
    if name_match:
        tags['name'] = name_match.group(1)
    
    # Тип
    type_match = re.search(r'@type\s+(\S+)', comment)
    if type_match:
        tags['type'] = type_match.group(1)
    
    # Группа
    group_match = re.search(r'@group\s+(.+?)(?:\s+@|$)', comment)
    if group_match:
        tags['group'] = group_match.group(1).strip()
    
    # Описание
    desc_match = re.search(r'@desc\s+(.+?)(?:\s+@|$)', comment)
    if desc_match:
        tags['description'] = desc_match.group(1).strip()
    
    # Hidden
    if '@hidden' in comment:
        tags['hidden'] = True
    
    # Readable (из @readable)
    rd_match = re.search(r'@readable\s+(\S+)', comment)
    if rd_match:
        tags['readable'] = rd_match.group(1).lower() == 'true'
    
    # Writable (из @writable)
    wr_match = re.search(r'@writable\s+(\S+)', comment)
    if wr_match:
        tags['writable'] = wr_match.group(1).lower() == 'true'
    
    # Unit
    unit_match = re.search(r'@unit\s+(.+?)(?:\s+@|$)', comment)
    if unit_match:
        tags['unit'] = unit_match.group(1).strip().strip('"')
    
    # Scale
    scale_match = re.search(r'@scale\s+(\S+)', comment)
    if scale_match:
        try:
            tags['scale'] = float(scale_match.group(1))
        except ValueError:
            pass
    
    # Min/Max
    min_match = re.search(r'@min\s+(\S+)', comment)
    if min_match:
        try:
            tags['min'] = int(min_match.group(1)) if min_match.group(1).isdigit() else float(min_match.group(1))
        except ValueError:
            pass
    
    max_match = re.search(r'@max\s+(\S+)', comment)
    if max_match:
        try:
            tags['max'] = int(max_match.group(1)) if max_match.group(1).isdigit() else float(max_match.group(1))
        except ValueError:
            pass
    
    # Bitfield
    if '@bitfield' in comment:
        tags['bitfield'] = True
    
    # Persistent
    if '@persistent' in comment:
        tags['persistent'] = True
    
    # Default
    default_match = re.search(r'@default\s+(\S+)', comment)
    if default_match:
        def_val = default_match.group(1)
        # Пробуем преобразовать в число или булево
        if def_val.lower() == 'true':
            tags['defaultValue'] = True
        elif def_val.lower() == 'false':
            tags['defaultValue'] = False
        elif def_val.isdigit():
            tags['defaultValue'] = int(def_val)
        else:
            tags['defaultValue'] = def_val.strip('"\'')
    
    # Enum values
    enum_match = re.search(r'@enum\s+(.+?)(?:\s+@|$)', comment)
    if enum_match:
        enum_str = enum_match.group(1).strip()
        enum_dict = {}
        for pair in enum_str.split(','):
            pair = pair.strip()
            if ':' in pair:
                k, v = pair.split(':', 1)
                enum_dict[k.strip()] = v.strip()
            else:
                enum_dict[str(len(enum_dict))] = pair
        if enum_dict:
            tags['enumValues'] = enum_dict
    
    return tags

def parse_h_file(filepath: str) -> List[Dict[str, Any]]:
    """Парсит .h файл и извлекает определения регистров."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    registers = []
    
    # Разбиваем на строки для построчного анализа
    lines = content.split('\n')
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Ищем начало комментария с @name
        if line.startswith('/*') and '@name' in line:
            # Собираем многострочный комментарий
            comment_lines = []
            while i < len(lines):
                comment_lines.append(lines[i])
                if '*/' in lines[i]:
                    break
                i += 1
            
            comment_text = '\n'.join(comment_lines)
            
            # Ищем следующую строку с REG
            i += 1
            while i < len(lines) and 'REG(' not in lines[i]:
                i += 1
            
            if i < len(lines):
                reg_line = lines[i].strip()
                
                # Извлекаем аргументы REG
                match = re.search(r'REG\(([^)]+)\)', reg_line)
                if match:
                    args_str = match.group(1)
                    # Учитываем вложенные скобки в default_value
                    args = []
                    current = ""
                    paren_count = 0
                    for ch in args_str:
                        if ch == '(':
                            paren_count += 1
                        elif ch == ')':
                            paren_count -= 1
                        
                        if ch == ',' and paren_count == 0:
                            args.append(current.strip())
                            current = ""
                        else:
                            current += ch
                    if current:
                        args.append(current.strip())
                    
                    if len(args) >= 6:
                        try:
                            addr = int(args[0], 0)
                            size = int(args[1])
                            readable = args[2] == 'true'
                            writable = args[3] == 'true'
                            persistent = args[4] == 'true'
                            default_val_str = args[5]
                            
                            # Парсим комментарий
                            tags = parse_comment(comment_text)
                            
                            # Собираем регистр
                            reg = {
                                'address': addr,
                                'size': size,
                                'readable': readable,
                                'writable': writable,
                                'persistent': persistent,
                            }
                            
                            # Если readable/writable переопределены в тегах
                            if 'readable' in tags:
                                reg['readable'] = tags.pop('readable')
                            if 'writable' in tags:
                                reg['writable'] = tags.pop('writable')
                            if 'persistent' in tags:
                                reg['persistent'] = tags.pop('persistent')
                            
                            # Добавляем все теги
                            reg.update(tags)
                            
                            # Если имя не указано, генерируем
                            if 'name' not in reg:
                                reg['name'] = f"REG_{addr:04X}"
                            
                            # Тип по умолчанию
                            if 'type' not in reg:
                                if size == 1:
                                    reg['type'] = 'UINT8'
                                elif size == 2:
                                    reg['type'] = 'UINT16'
                                elif size == 4:
                                    reg['type'] = 'UINT32'
                                else:
                                    reg['type'] = 'BYTE_ARRAY'
                            
                            # Парсим значение по умолчанию если не указано в тегах
                            if 'defaultValue' not in reg and default_val_str != 'nullptr':
                                val = parse_default_value(default_val_str, reg['type'])
                                if val is not None:
                                    reg['defaultValue'] = val
                            
                            registers.append(reg)
                            print(f"Parsed: {reg['name']} at 0x{addr:04X}")
                        except Exception as e:
                            print(f"Error parsing REG: {e}")
                            print(f"Args: {args}")
        
        i += 1
    
    return registers

def generate_json(registers: List[Dict], device_type: str, version: str = "1.0") -> Dict:
    """Генерирует полный JSON с метаданными."""
    # Собираем группы
    groups_set = set()
    for reg in registers:
        if 'group' in reg:
            groups_set.add(reg['group'])
    
    groups = [{"name": g, "description": f"Настройки группы {g}"} for g in sorted(groups_set)]
    
    return {
        "deviceType": device_type,
        "version": version,
        "registers": registers,
        "groups": groups
    }

def main():
    if len(sys.argv) < 4:
        print("Usage: python h_to_json.py <input.h> <output.json> <device_type>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    device_type = sys.argv[3]
    
    print(f"Parsing {input_file}...")
    registers = parse_h_file(input_file)
    print(f"Found {len(registers)} registers")
    
    if not registers:
        print("WARNING: No registers found!")
        # Выведем первые 20 строк файла для отладки
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()[:30]
            print("First 30 lines of file:")
            for i, line in enumerate(lines, 1):
                print(f"{i:3}: {line.rstrip()}")
    
    json_data = generate_json(registers, device_type)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(json_data, f, ensure_ascii=False, indent=2)
    
    print(f"Generated {output_file} with {len(registers)} registers")

if __name__ == '__main__':
    main()