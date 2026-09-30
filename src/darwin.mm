/*
    Copyright © 2026 Michael Shields

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE.txt file.
*/

#include <nanogui/common.h>
#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <unistd.h>

namespace nanogui {

std::string file_dialog(const std::vector<std::pair<std::string, std::string>> &filetypes, bool save) {
    @autoreleasepool {
        NSMutableArray<UTType *> *types = [NSMutableArray new];
        for (const auto &filetype : filetypes) {
            UTType *type = [UTType typeWithFilenameExtension:
                [NSString stringWithUTF8String:filetype.first.c_str()]];
            if (!type)
                throw std::runtime_error("Unknown file type: " + filetype.first);
            [types addObject:type];
        }

        NSSavePanel *panel;
        if (save) {
            panel = [NSSavePanel savePanel];
        } else {
            NSOpenPanel *openPanel = [NSOpenPanel openPanel];
            openPanel.canChooseFiles = YES;
            openPanel.canChooseDirectories = NO;
            openPanel.allowsMultipleSelection = NO;
            panel = openPanel;
        }
        panel.allowedContentTypes = types;
        if ([panel runModal] == NSModalResponseOK)
            return std::string(panel.URL.path.UTF8String);
        return {};
    }
}

void chdir_to_bundle_parent() {
    @autoreleasepool {
        NSString *path = NSBundle.mainBundle.bundlePath.stringByDeletingLastPathComponent;
        if (chdir(path.fileSystemRepresentation) != 0)
            throw std::runtime_error("Could not change to application directory");
    }
}

}
