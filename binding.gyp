{
    "targets": [
        {
            "target_name": "poppler",
            "sources": [
                "src/NodePopplerDocument.cc",
                "src/NodePopplerPage.cc",
                "src/iconv_string.cc"
            ],
            "libraries": [
                "<!@(pkg-config --libs poppler)"
            ],
            "cflags": [
                "<!@(pkg-config --cflags poppler)"
            ]
        }
    ]
}