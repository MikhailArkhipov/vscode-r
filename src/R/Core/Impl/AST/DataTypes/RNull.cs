// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.R.Core.AST.DataTypes.Definitions;

namespace Microsoft.R.Core.AST.DataTypes {
    public sealed class RNull : RObject, IRVector {
        public static RNull Null { get; } = new RNull();

        public int Length => 0;

        public RMode Mode => RMode.Null;

        public override string ToString() => "NULL";
    }
}
