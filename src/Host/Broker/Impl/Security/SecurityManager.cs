// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Security.Claims;
using System.Threading.Tasks;
using Odachi.AspNetCore.Authentication.Basic;

namespace Microsoft.R.Host.Broker.Security {
    public class SecurityManager {
        public Task SignInAsync(BasicSignInContext context) {
            var claims = new[] { new Claim(ClaimTypes.Anonymous, "") };
            var claimsIdentity = new ClaimsIdentity(claims, context.Scheme.Name);
            context.Principal = new ClaimsPrincipal(claimsIdentity);
            return Task.CompletedTask;
        }
    }
}
