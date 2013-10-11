{
    "variables": {
        "major_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[0];')",
        "minor_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[1];')",
        "micro_version": "<!(node -pe 'v=process.versions.node.split(\".\"); v[2];')"
    },
    "targets": [
        {
            "target_name": "poppler",
            "sources": [
                "src/poppler.cc",
                "src/NodePopplerDocument.cc",
                "src/NodePopplerPage.cc",
                "src/iconv_string.cc"
            ],
            "libraries": [
                "<!@(pkg-config --libs poppler)"
            ],
            "cflags": [
                "<!@(pkg-config --cflags poppler)"
            ],
            "defines": [
                "NODE_VERSION_MAJOR_<(major_version)",
                "NODE_VERSION_MINOR_<(minor_version)",
                "NODE_VERSION_MICRO_<(micro_version)"
            ]
        }
    ]
}
