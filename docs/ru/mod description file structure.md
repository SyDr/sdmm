# Структура файла `mod.json`

### Замечания

Любые секции, в которых что-то может быть переведено определяются следующим образом:

    "<имя секции>": {
      "en_US" : "на английском",
      "ru_RU" : "на русском",
      "<код языка>" : "на другом языке"
    }
Эти секции будут упоминаться как `"<имя секции>" : lng` для сокращения записи.  
Если значение для текущего языка отсутствует, пустое или имеет невалидное значение, вариант из `en_US` будет использоваться вместо него.  
Все пути относительные и используют каталог мода в качестве базового.

### Основная информация о моде
    "caption": lng,
    "description": {
       "full": lng,
       "short": lng
    }
Caption - не нуждается в объснении (если не указано, ММ будет использовать имя каталога с модом).  
Full description - указывает путь к файлу с описанием (простой текст). Если не указано, readme.txt будет использован.  
Short description - напрямую указывает коротенькое описание мода, которое нигде не используется.

    "author": "SyDr",

Имя автора (или авторов).

    "homepage": "http://wforum.heroes35.net",  

Адрес домашней страницы мода.  

    "icon": {
      "file": "icon.ico",
      "index": 0
    },

Если иконка не указана, будет использована стандартная. Индекс начинается с 0.  

    "video" : "w46whZ-SKQ4"

Video - id видео с YouTube, пока нигде не используется.
  
    "category": "gameplay"

Основная категория мода. См. раздел `category` в файлах перевода, чтобы увидеть доступные значения.

### Compatibility / Mod load order optimization (`compatibility` tag)
When looking for a some file, Era II will start from mod with greatest priority. I will refer to load order - exact opposite thing. For example if i say `Mod 2` loaded after `Mod 1` this means that `Mod 2` will have greater priority, and game will took files from it instead of `Mod 1` where it possible. Don't be confused :).

    "incompatible" : ["<Mod ID>"],
    "required" : ["Mod ID"],
    "hints" : (1)[
        (2)["<Mod ID 1>", (3)["<Mod ID 2>", "<Mod ID 3>"], "@mod"],
        ["@mod", "Mod ID"],
        ["<Mod ID 1>", "<Mod ID 2>"]
    ]    
    "priority" : 0
`incompatible` lists mods, with which game cannot be loaded or this action will not make any sense.  
`required` lists mods, without which game cannot be loaded or this action will not make any sense (this will not restrict mod load order).  
`hints` is a list (1) of ordered lists (2) of unordered lists (3). Yep. It's simple. Read this as:

 * `@mod` - just a shortcut to current mod ID (directory name)
 * `["<Mod ID 1>", "<Mod ID 2>"]` - `Mod 2` should be loaded after `Mod 1`
 * `["<Mod ID 1>", ["<Mod ID 2>", "<Mod ID 3>"]]` - `Mod 2` and `Mod 3` should be loaded after `Mod 1` (this not restricts `Mod 2` and `Mod 3` load order)

`priority` used for additional mod load order optimizations. It will be ignored if mods are successfully sorted by other info.
