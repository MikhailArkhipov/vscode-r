// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.IO;
using System.Linq;
using Microsoft.Common.Core.IO;
using NSubstitute;
using NSubstitute.ExceptionExtensions;

namespace Microsoft.Common.Core.Test.StubFactories {
    [ExcludeFromCodeCoverage]
    public static class DirectoryInfoStubFactory {
        public static IDirectoryInfo Create(IFileSystem fileSystem, string path) {
            var di = Substitute.For<IDirectoryInfo>();
            di.FullName.Returns(path);
            di.Exists.Returns(true);
            di.Parent.Returns((IDirectoryInfo)null);
            di.EnumerateFileSystemInfos().Returns(new List<IFileSystemInfo>());

            fileSystem.GetDirectoryInfo(path).Returns(di);
            fileSystem.DirectoryExists(path).Returns(true);

            fileSystem.GetDirectoryInfo(path + Path.DirectorySeparatorChar).Returns(di);
            fileSystem.DirectoryExists(path + Path.DirectorySeparatorChar).Returns(true);

            return di;
        }

        public static IDirectoryInfo Delete(IFileSystem fileSystem, string path) {
            var di = Substitute.For<IDirectoryInfo>();
            di.FullName.Returns(path);
            di.Exists.Returns(false);
            di.Parent.Returns((IDirectoryInfo)null);
            di.EnumerateFileSystemInfos().ThrowsForAnyArgs<DirectoryNotFoundException>();

            fileSystem.GetDirectoryInfo(path).Returns(di);
            fileSystem.DirectoryExists(path).Returns(false);

            fileSystem.GetDirectoryInfo(path + Path.DirectorySeparatorChar).Returns(di);
            fileSystem.DirectoryExists(path + Path.DirectorySeparatorChar).Returns(false);

            return di;
        }

        public static IFileSystemInfo FromIndentedString(IFileSystem fileSystem, string rootPath, string indentedString) {
            var lines = indentedString.Split('\r', '\n');
            var folderIndents = new Stack<int>();
            folderIndents.Push(-1);
            var root = Create(fileSystem, rootPath);
            var directory = root;

            foreach (var line in lines.Where(l => !string.IsNullOrWhiteSpace(l))) {
                var indent = GetIndent(line);
                var name = line.Trim();
                var isFolder = IsFolder(name);

                if (isFolder) {
                    // Find a folder for current item
                    while (indent <= folderIndents.Peek()) {
                        folderIndents.Pop();
                        directory = directory.Parent;
                    }

                    var path = Path.Combine(directory.FullName, name.Substring(1, name.Length - 2));
                    var child = Create(fileSystem, path);

                    AddToDirectory(directory, child);

                    folderIndents.Push(indent);
                    directory = child;
                } else {
                    // Find a folder for current item
                    while (indent <= folderIndents.Peek()) {
                        folderIndents.Pop();
                        directory = directory.Parent;
                    }

                    var path = Path.Combine(directory.FullName, name);
                    IFileSystemInfo child = FileInfoStubFactory.Create(fileSystem, path);

                    AddToDirectory(directory, child);
                }
            }

            var children = (List<IFileSystemInfo>)root.EnumerateFileSystemInfos();

            if (children.Count != 1) {
                return root;
            }

            var result = children.First();
            children.Remove(result);
            return result;
        }

        private static void AddToDirectory(IDirectoryInfo directory, IFileSystemInfo child) {
            var children = (List<IFileSystemInfo>)directory.EnumerateFileSystemInfos();
            var index = children.BinarySearch(child, Comparer<IFileSystemInfo>.Create((x, y) => string.Compare(x.FullName, y.FullName, StringComparison.OrdinalIgnoreCase)));
            if (index >= 0) {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Item with name {0} already exists in folder {1}", child.FullName, directory.FullName));
            }

            if (child.TraverseBreadthFirst(f => (f as IDirectoryInfo)?.EnumerateFileSystemInfos()).Contains(directory)) {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Circular dependency. Item {0} is a parent of {1} already", child.FullName, directory.FullName));
            }

            var directoryChild = child as IDirectoryInfo;
            if (directoryChild != null) {
                var oldParent = (List<IFileSystemInfo>)directoryChild.Parent?.EnumerateFileSystemInfos();
                oldParent?.Remove(directoryChild);
                directoryChild.Parent.Returns(directory);
            } else {
                var fileChild = (IFileInfo)child;
                var oldParent = (List<IFileSystemInfo>)fileChild.Directory?.EnumerateFileSystemInfos();
                oldParent?.Remove(fileChild);
                fileChild.Directory.Returns(directory);
            }

            children.Insert(~index, child);
        }

        private static int GetIndent(string s) {
            for (var i = 0; i < s.Length; i++) {
                if (!char.IsWhiteSpace(s, i)) {
                    return i;
                }
            }

            return s.Length;
        }

        private static bool IsFolder(string s) {
            return s[0] == '[' && s[s.Length - 1] == ']';
        }
    }
}
