from building import *

cwd = GetCurrentDir()
path = [cwd+'/inc']
src  = Glob('src/*.c')
 
group = DefineGroup('rs485', src, depend = ['PKG_USING_RS485'], CPPPATH = path)

Return('group')