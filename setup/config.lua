--local pl = require 'pl'
local pp = require 'pp'

local config = {
  log_file = 'log.txt',
  inno_setup_path = [[C:\Program Files (x86)\Inno Setup 6\iscc.exe]],
  release_files = {
   '../icons',
   '../lng',
   '../vc_mswu/mm.exe',
   '../LICENSE',
  },
  mm_version = '0.98.0.beta',
  ftp = {
    upload = true,
    host = 'wakeofgods.org',
    port = '21',
    user = 'mm@wakeofgods.org',
    pass = '',
  }
}


return config
