env = Environment()

asan = False
if asan:
    env.Append(CXXFLAGS=['-fsanitize=address', '-fno-omit-frame-pointer'])
    env.Append(LINKFLAGS=['-fsanitize=address'])

env.Append(CXXFLAGS=['-std=gnu++0x', '-g'])
# env.Append(CXXFLAGS=['-Wall'])

unneeded_srcs = [
    # Files that aren't used even in the win32 build
    'AppControl.cpp',
    'BodySlide.cpp',
    # Built in the win32 build, but not used anywhere
    'GroupManager.cpp',
    'SliderMaker.cpp',
    # Not needed for linux
    'stdafx.cpp',
]
srcs = [
    'AABBTree.cpp',
    'Anim.cpp',
    'AsyncMonitor.cpp',
    'Automorph.cpp',
    'BodySlideApp.cpp',
    'ConfigurationManager.cpp',
    'DiffData.cpp',
    'GLShader.cpp',
    'GLSurface.cpp',
    'Mesh.cpp',
    'NifBlock.cpp',
    'NifFile.cpp',
    'Object3d.cpp',
    'ObjFile.cpp',
    'OutfitProject.cpp',
    'OutfitStudio.cpp',
    'PresetSaveDialog.cpp',
    'PreviewWindow.cpp',
    'ResourceLoader.cpp',
    'SliderCategories.cpp',
    'SliderData.cpp',
    'SliderGroup.cpp',
    'SliderManager.cpp',
    'TweakBrush.cpp',
    'wxStateButton.cpp',

    'tinystr.cpp',
    'tinyxml.cpp',
    'tinyxmlerror.cpp',
    'tinyxmlparser.cpp',
]

wx_include_paths = [
    '/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-3.0',
    '/usr/include/wx-3.0',
]
wx_libs = [
    'wx_gtk2u_xrc-3.0',
    'wx_gtk2u_html-3.0',
    'wx_gtk2u_qa-3.0',
    'wx_gtk2u_adv-3.0',
    'wx_gtk2u_core-3.0',
    'wx_gtk2u_gl-3.0',
    'wx_baseu_xml-3.0',
    'wx_baseu_net-3.0',
    'wx_baseu-3.0',
]
wx_defines = ['WXUSINGDLL', '__WXGTK__']
wx_defines_dict = {'_FILE_OFFSET_BITS': '64'}
env.Append(CPPPATH=wx_include_paths)
env.Append(CPPDEFINES=wx_defines)
env.Append(CPPDEFINES=wx_defines_dict)
env.Append(LIBS=wx_libs)

boost_libs = ['boost_filesystem', 'boost_system']
env.Append(LIBS=boost_libs)

gl_libs = ['GL', 'GLEW', 'GLU', 'SOIL']
env.Append(LIBS=gl_libs)

env.Append(CXXFLAGS=['-pthread'])

env.Program('bodyslide', srcs)
