
mod.json file structure
---------------------------

# Structure
```json
{
  "name" : lng,
  "description" : lng,
  "author" : "SyDr",
  "homepage" : "https://...",
  "support" : "https://...",
  "support" : [ "you", "can", "include", "multiple", "entries", "too" ],
  "icon" : "icon.png",
  "version" : "1.0",
  "category" : "gameplay",
  "plugins" : {
    "example.era" : {
      "caption" : lng,
      "description" : lng
    }
  },
  "compatibility" : {
    "requires" : [ "WoG", "Test1" ],
    "load_after" : [ "WoG", "Test2" ],
    "incompatible" : [ "Test3" ]
  }
}
```

## Notes
Any section, defined as `"section": lng` means that section is defined as following instead (to reduce boilerplate)
```
"section": {
  "en" : "value",
  "ru" : "value",
  "<code>" : "value"
}
```
MM will try to load entry for current language first. If it's not possible - english will be used instead. 
All paths are relative to mod directory.
MM will try to load data from deprecated fields if data is not present in expected format.

### name
Displayed name of the mod.

### description
Name of the file with mod description (displayed at the right side in main window. You can use .md files too.

### author
Author[s] of the mod.

### homepage
Homepage for the mod. Available from mod context menu in the main view.

### support
Either single link or an array of links. Displayed in the main view when clicked on ♥ icon.

### category
See files in "lng" directory for list of available values.

### plugins
Optional captions and descriptions for files managed by the plugin dialog. Each key is the canonical,
enabled filename (for example `example.era`, even when the file is currently named `example.era.off`).
Both `caption` and `description` use the same localized `lng` shape as the mod name. Plain strings are
also accepted. Missing captions fall back to the canonical filename; missing descriptions use the
manager's localized "No description available" text.

To support both the legacy AutoIt manager and the current manager, use localized objects with the
legacy language codes. The current manager recognizes these codes as fallbacks:

```json
"plugins" : {
  "amethyst2_4.dll" : {
    "default" : true,
    "caption" : {
      "en_US" : "Amethyst 2",
      "ru_RU" : "Аметист 2",
      "Cn_Simp" : "紫晶 2",
      "zh_CN" : "紫晶 2"
    },
    "description" : {
      "en_US" : "Enables the Amethyst gameplay extensions.",
      "ru_RU" : "Включает игровые расширения Amethyst.",
      "Cn_Simp" : "启用紫晶游戏扩展。",
      "zh_CN" : "启用紫晶游戏扩展。"
    }
  }
}
```

The current language codes are `en`, `ru`, and `zh`. Their legacy metadata fallbacks are `en_US`,
`ru_RU`, and, for Chinese, `Cn_Simp` followed by `zh_CN`. If both Chinese legacy keys are present,
`Cn_Simp` takes precedence.

The plugin key must match the enabled filename exactly for the legacy manager and must never include
the terminal `.off`. The description value is display text, not the name of a description file.
`default` is retained for the legacy format; the current direct-rename manager ignores it.

The manager discovers non-empty `.era`, `.dll`, and `.bin` files directly inside `EraPlugins`, plus
all non-empty regular files directly inside its `BeforeWoG` and `AfterWoG` subdirectories. Folder-name
casing is ignored.
Disabling a plugin appends `.off` to its filename, and enabling it removes that terminal suffix.
Nested folders and zero-byte blocker files are not managed.

### compatibility
When looking for a some file, Era II will start from mod with greatest priority. I will refer to load order - exact opposite thing. For example if i say `Mod 2` loaded after `Mod 1` this means that `Mod 2` will have greater priority, and game will took files from it instead of `Mod 1` where it possible. Don't be confused :).

`incompatible` lists mods, with which game cannot be loaded or this action will not make any sense in combination with this mod.  
`requires` lists mods, without which game cannot be loaded or this action will not make any sense in combination with this mod (this will also automatically adds this mod to load order).  
`load_after` lists mods, which should have less priority than this mod (i.e. game will load files from this mod if they are sorted correctly).

`WoG` is implicitly added to `requires` and `load_after`, unless you modify them (i.e., you probably want add WoG to `requires` too).

## Legacy mod_info.ini

When `mod.json` is absent or invalid, the manager reads the legacy `[info]` section from
`mod_info.ini`. Supported fields are `Caption.<language>`, `Description File.<language>`, `Version`,
`Author`, `Homepage`, `Icon File`, `Priority`, and `Category`. Localized values use the current language,
its legacy language codes in configured order, English, and finally the unqualified `Caption` or
`Description File` value.
The description field is a filename relative to the mod directory.
