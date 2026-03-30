#pragma once

// FreeArc arc.groups classification

#include <cstring>
#include <string>

enum FileGroup {
    GRP_COMPRESSED = 0, // already compressed — store raw
    GRP_PRECOMP,        // zip/png/pdf — precomp can help
    GRP_JPG,            // jpeg
    GRP_WAV,            // raw audio
    GRP_BMP,            // raw bitmaps
    GRP_TEXT,            // source code, markup, config
    GRP_EXE,            // executables — BCJ+lzma
    GRP_OBJ,            // object files
    GRP_BINARY,         // default catch-all
    GRP_COUNT
};

static const char *grpName[] = {
    "compressed","precomp","jpg","wav","bmp","text","exe","obj","binary"
};

static bool extMatch(const char *ext, const char *list) {
    char buf[4096]; strncpy(buf, list, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    for (char *t = strtok(buf," "); t; t = strtok(nullptr," "))
        if (strcasecmp(ext, t) == 0) return true;
    return false;
}

static FileGroup classifyExt(const std::string &name) {
    auto dot = name.rfind('.');
    if (dot == std::string::npos) return GRP_TEXT; // extensionless -> text
    std::string ext = name.substr(dot+1);
    const char *e = ext.c_str();

    // already compressed — storing only
    if (extMatch(e, "mp3 mp4 mkv avi flac ogg opus aac wma wmv m4a m4v "
                     "webm webp avif heif heic "
                     "7z rar zip gz bz2 xz zst lz4 lzma lzo "
                     "arc arj lzh cab zoo pak hpk "
                     "pmd pmm pms ccm ccmx djvu chm "
                     "br snappy")) return GRP_COMPRESSED;

    if (extMatch(e, "jpg jpeg jfif")) return GRP_JPG;

    // precomp candidates (containers with internal compression)
    if (extMatch(e, "pdf swf zip jar png gif gz tgz svgz "
                     "docx docm dotx dotm xlsx xlsm xltx xltm xlam "
                     "pptx pptm potx potm ppam ppsx ppsm "
                     "odt ott ods ots odg odp odf odb oxt "
                     "apk ipa xpi crx nupkg whl egg "
                     "fb2z sis gadget "
                     "pk3 pk4 pak")) return GRP_PRECOMP;

    if (extMatch(e, "wav wave pcm aif aifc aiff au snd raw")) return GRP_WAV;
    if (extMatch(e, "bmp tif tiff tga wbm pgm pnm ppm dds")) return GRP_BMP;

    if (extMatch(e, "exe dll so com scr sfx ocx bpl dpl "
                     "sys drv vxd ovr ovl")) return GRP_EXE;

    if (extMatch(e, "obj o a lib dcu")) return GRP_OBJ;

    if (extMatch(e, "txt asc lng css htm html xml xsl json yaml yml toml "
                     "md rst tex log ini cfg conf "
                     "c cpp cxx cc h hpp hxx ipp "
                     "cs java js ts jsx tsx "
                     "py rb pl pm lua tcl sh bash zsh fish "
                     "php asp asm s inc "
                     "pas dfm lfm lpi "
                     "hs lhs ml mli erl hrl scm "
                     "sql csv rtf "
                     "bat cmd vbs ps1 "
                     "makefile cmake "
                     "rc idl dsp dsw vcproj sln csproj")) return GRP_TEXT;

    return GRP_BINARY;
}

static std::string methodForGroup(const char *userMethod, FileGroup grp) {
    switch (grp) {
        case GRP_COMPRESSED:
        case GRP_JPG:
            return "storing";

        case GRP_PRECOMP:
            return "storing";

        case GRP_WAV:
        case GRP_BMP:
            // TODO: delta+lzma chain
            return userMethod;

        default:
            return userMethod;
    }
}
