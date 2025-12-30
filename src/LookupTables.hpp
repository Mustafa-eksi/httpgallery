#pragma once

static const std::unordered_map<std::string, std::string> mime_types = {
    // Web Essentials (Updated)
    {"html",  "text/html; charset=utf-8"},
    {"htm",   "text/html; charset=utf-8"},
    {"css",   "text/css"},
    {"js",    "text/javascript"},
    {"mjs",   "text/javascript"}, // JavaScript modules
    {"json",  "application/json"},
    {"jsonld","application/ld+json"},
    {"php",   "text/x-php"},

    // Images (Updated)
    {"png",   "image/png"},
    {"jpg",   "image/jpeg"},
    {"jpeg",  "image/jpeg"},
    {"gif",   "image/gif"},
    {"svg",   "image/svg+xml"},
    {"ico",   "image/x-icon"},
    {"webp",  "image/webp"},
    {"avif",  "image/avif"}, // Modern high-efficiency format
    {"tiff",  "image/tiff"},
    {"tif",   "image/tiff"},
    {"bmp",   "image/bmp"},

    // Video & Audio
    {"mp4",   "video/mp4"},
    {"webm",  "video/webm"},
    {"ogv",   "video/ogg"},
    {"mov",   "video/quicktime"},
    {"avi",   "video/x-msvideo"},
    {"mp3",   "audio/mpeg"},
    {"wav",   "audio/wav"},
    {"ogg",   "audio/ogg"},
    {"m4a",   "audio/x-m4a"},
    {"aac",   "audio/aac"},

    // Microsoft Office Documents
    {"doc",   "application/msword"},
    {"docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls",   "application/vnd.ms-excel"},
    {"xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt",   "application/vnd.ms-powerpoint"},
    {"pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

    // Documents & Data
    {"pdf",   "application/pdf"},
    {"txt",   "text/plain"},
    {"xml",   "application/xml"},
    {"csv",   "text/csv"},
    {"rtf",   "application/rtf"},
    {"md",    "text/markdown"},
    {"yaml",  "text/yaml"},
    {"yml",   "text/yaml"},

    // Fonts
    {"woff",  "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf",   "font/ttf"},
    {"otf",   "font/otf"},
    {"eot",   "application/vnd.ms-fontobject"},

    // Archives & Binary
    {"zip",   "application/zip"},
    {"gz",    "application/gzip"},
    {"rar",   "application/vnd.rar"},
    {"7z",    "application/x-7z-compressed"},
    {"tar",   "application/x-tar"},
    {"bin",   "application/octet-stream"},
    {"exe",   "application/octet-stream"},
    {"dll",   "application/octet-stream"}
};
