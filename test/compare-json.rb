#!/usr/bin/env ruby

require 'rubygems'
require 'json'

def usage()
  STDERR.write("usage:\n")
  STDERR.write("compare-json.rb [--ignore-log] file1 file2\n")
  STDERR.write("file1 or file2 can be '-' for standard input\n")
  exit 1
end

files = []

ignore_log = false

ARGV.each do |arg|
  if arg == '--ignore-log'
    ignore_log = true
  elsif arg == '-'
    files << STDIN
  else
    files << File.open(arg)
  end
end

files.size == 2 || usage()

json = files.map {|f| JSON.parse(f.read())}

if ignore_log 
  json.map {|j| j.delete("log")}
end

if json[0] != json[1]
  STDERR.write("JSON differs\n");
  exit 1
else
  STDERR.write("JSON same\n");
  exit 0
end
