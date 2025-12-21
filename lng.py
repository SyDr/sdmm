import json
import os
import re
import msvcrt
from typing import Dict, Any, List, Tuple, Set

SRC_DIR = "src"
LNG_DIR = "lng"

LNG_FILES = (os.path.join(LNG_DIR, "en.json"), os.path.join(LNG_DIR, "ru.json"))
SOURCE_EXTENSIONS = ('.cpp', '.h', '.hpp')

MAYBE_USED = ('category/',
    'column/', 
    'dialog/settings/configure_main_view/archived_mods_value/', 
    'dialog/settings/configure_main_view/managed_mods_value/', 
    'dialog/settings/interface_label/',
    'dialog/settings/interface_size/',
    'dialog/settings/mod_description_control/',
    'dialog/settings/update_mode/'
)

def load_json(filename: str):
    with open(filename, 'r', encoding='utf-8') as file:
        return json.load(file)

def save_json(filename: str, data: Dict[str, Any]) -> None:
    with open(filename, 'w', encoding='utf-8') as file:
        json.dump(data, file, indent=2, ensure_ascii=False)

def flatten_json(data: Dict[str, Any], parent_key: str = '', sep: str = '/') -> Dict[str, Any]:
    items = []

    if isinstance(data, dict):
        for k, v in data.items():
            new_key = f"{parent_key}{sep}{k}" if parent_key else k
            if isinstance(v, (dict, list)):
                items.extend(flatten_json(v, new_key, sep=sep).items())
            else:
                items.append((new_key, v))
    elif isinstance(data, list):
        for i, v in enumerate(data):
            new_key = f"{parent_key}{sep}{i}" if parent_key else str(i)
            if isinstance(v, (dict, list)):
                items.extend(flatten_json(v, new_key, sep=sep).items())
            else:
                items.append((new_key, v))

    return dict(items)

def unflatten_json(data: Dict[str, Any], sep: str = '/') -> Dict[str, Any]:
    result = {}
    
    for key, value in data.items():
        parts = key.split(sep)
        current = result
        for part in parts[:-1]:
            if part not in current:
                current[part] = {}
            current = current[part]
        current[parts[-1]] = value

    return result

def replace_in_files(pattern: str, replacement: str) -> None:
    compiled_pattern = r'"' + re.escape(pattern) + r'"_lng'
    
    def replace_func(match):
        return '"' + replacement + '"_lng'

    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith(SOURCE_EXTENSIONS):
                filepath = os.path.join(root, file)
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                new_content = re.sub(compiled_pattern, replace_func, content)
                
                if new_content != content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)

def load_language_files() -> List[Tuple[str, Dict[str, Any]]]:
    result = list()

    for path in LNG_FILES:
        data = load_json(path)
        result.append((path, flatten_json(data)))

    return result

def process_language_keys(source_key: str, target_key: str, jsons: List[Tuple[str, Dict[str, Any]]]) -> None:
    replace_in_files(source_key, target_key)

    for path, data in jsons:
        if source_key in data:
            data[target_key] = data.pop(source_key)
        else:
            data[target_key] = source_key
        
        save_json(path, unflatten_json(data))

def find_used_keys() -> Set[str]:
    used_keys = set()
    
    # Pattern to match "_lng" suffix
    pattern = re.compile(r'"([^"]+)"_lng')
    
    for root, dirs, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith(SOURCE_EXTENSIONS):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                    
                    # Find all matches
                    matches = pattern.findall(content)
                    used_keys.update(matches)
                except (IOError, OSError) as e:
                    print(f"Warning: Could not read {filepath}: {e}")
    
    return used_keys

def get_all_keys(json_path: str) -> Set[str]:
    try:
        data = load_json(json_path)
        flattened = flatten_json(data)
        return set(flattened.keys())
    except (IOError, OSError, json.JSONDecodeError) as e:
        print(f"Warning: Could not read {json_path}: {e}")
        return set()

def print_unused_keys():
    used_keys = find_used_keys()
    
    for path in LNG_FILES:
        keys = get_all_keys(path)
        
        unused_keys = keys - used_keys
        
        unused_keys = {item for item in unused_keys if not item.startswith(MAYBE_USED)}
        
        print(f"Unused keys in {path} file:")
        for key in sorted(unused_keys):
            print(f"  {key}")
        
    print("Press any key...")
    msvcrt.getch()

def update_key():
    while True:
        try:
            str1 = input("Source: ")
            str2 = input("Target: ")
            process_language_keys(str1, str2, load_language_files())
        except EOFError:
            break
        except KeyboardInterrupt:
            break

def clear_screen():
    os.system('cls')

def display_menu(current_selection):
    clear_screen()
    menu_options = [
        "Print unused keys",
        "Update key",
        "Exit (or Ctrl+C)"
    ]
    
    for i, option in enumerate(menu_options):
        if i == current_selection:
            print(f"> {option}")
        else:
            print(f"  {option}")
                
def get_key():
    while True:
        if msvcrt.kbhit():
            key = msvcrt.getch()
            return key

def main():
    current_selection = 0
    try:
        while True:
            display_menu(current_selection)
            
            key = get_key()
            
            if key == b'\xe0':
                key = msvcrt.getch()
                if key == b'H':
                    current_selection = (current_selection - 1) % 3
                elif key == b'P':
                    current_selection = (current_selection + 1) % 3
            elif key == b'\r':
                if current_selection == 0:
                    print_unused_keys()
                elif current_selection == 1:
                    update_key()
                elif current_selection == 2:
                    break;
            elif key == b'\x03':
                break;
    except EOFError:
        pass
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
