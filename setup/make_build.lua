local winapi = require 'winapi'
local proc = require'proc'
--local pl = require 'pl'
local ftp = require 'socket.ftp'
local ltn12 = require 'ltn12'
local pp = require 'pp'
local fs = require 'fs'

local function find_msbuild()
  local command = [[""%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe"]]
  local f = assert(io.popen(command, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  
  s = string.gsub(s, '[\n\r]+', '')
  
  assert(s:len() > 0)
  
  return s
end

local function build(t, msbuild_path)
  local command = string.format([[""%s" /noconsolelogger /fileLogger /flp:logfile=log.txt /property:Configuration=Release "..\\SD MM.sln""]], msbuild_path)
  local f = assert(io.popen(command, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  
  return nil
end

local function make_setup(t)
  local command = string.format([[""%s" setup.iss"]], t.inno_setup_path, t.log_file)
  local f = assert(io.popen(command, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  
  return nil
end

local function create_envinronment(t)
  local step = 0
  local pr = nil
  return function(f, d)
    step = step + 1
    print(string.format("%d: %s", step, d))
    pr = f(t, pr)
  end
end

local config = require 'config'
local exec = create_envinronment(config)

exec(find_msbuild, "find_msbuild")
exec(build, "build")
exec(make_setup, "make_setup")
