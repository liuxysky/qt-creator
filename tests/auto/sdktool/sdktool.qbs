import qbs

QtcAutotest {
    name: "sdktool autotest"

    Group {
        name: "Test sources"
        files: "tst_sdktool.cpp"
    }

    cpp.defines: base.concat(['SDKTOOL_DIR="' + qbs.installRoot + '/' + project.ide_libexec_path + '"'])
}
