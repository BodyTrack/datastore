#!/usr/bin/env ruby

require 'json'

testdir = File.dirname(__FILE__)
rootdir = "#{testdir}/.."
store = "#{testdir}/TestGetTile.kvs"

def sys_print(cmd)
  puts cmd
  return `#{cmd}`
end

#sys_print("rm -rf #{store}")
#sys_print("mkdir #{store}")
#sys_print("#{rootdir}/import #{store} 1 A_Cheststrap #{rootdir}/testdata/anne-cheststrap-bug/4e386a43.bt")


tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 5 80097")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 6 40048")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 7 20024")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 8 10012")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 9 5006")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 10 2503")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 11 1251")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 12 625")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 13 312")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 4 160195")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 3 320390")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 2 640781")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 1 1281562")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration 0 2563125")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -1 5126250")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -2 10252500")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -3 20505001")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -4 41010002")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -5 82020004")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -6 164040008")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -7 328080016")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -8 656160033")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -9 1312320067")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -10 2624640134")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -11 5249280268")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -12 10498560536")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -13 20997121072")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -14 41994242144")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -15 83988484288")
tile = sys_print("#{rootdir}/gettile #{store} 1 A_Cheststrap.Respiration -16 167976968576")

#puts tile


