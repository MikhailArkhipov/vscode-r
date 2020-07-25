// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Claims;
using System.Security.Principal;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Logging;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using Microsoft.R.Host.Broker.Interpreters;
using Microsoft.R.Host.Broker.Logging;
using Microsoft.R.Host.Broker.Pipes;
using Microsoft.R.Host.Broker.Services;
using Microsoft.R.Host.Protocol;

namespace Microsoft.R.Host.Broker.Sessions {
    public class SessionManager {
        private readonly LoggingOptions _loggingOptions;
        private readonly ILogger _messageLogger;
        private readonly IRHostProcessService _processService;
        private readonly IApplicationLifetime _applicationLifetime;
        private readonly ILogger _sessionLogger;

        private readonly List<Session> _sessions = new List<Session>();

        public SessionManager(IRHostProcessService processService
            , IApplicationLifetime applicationLifetime
            , IOptions<LoggingOptions> loggingOptions
            , ILogger<Session> sessionLogger
            , ILogger<MessagePipe> messageLogger) {

            _loggingOptions = loggingOptions.Value;
            _processService = processService;
            _applicationLifetime = applicationLifetime;
            _sessionLogger = sessionLogger;

            if (_loggingOptions.LogPackets) {
                _messageLogger = messageLogger;
            }
        }

        public IEnumerable<Session> GetSessions() {
            lock (_sessions) {
                return _sessions.ToArray();
            }
        }

        public Session GetSession(string id) {
            lock (_sessions) {
                return _sessions.FirstOrDefault(session => session.Id == id);
            }
        }

        private List<Session> GetOrCreateSessionList() {
            lock (_sessions) {
                return _sessions;
            }
        }

        public Session CreateSession(string id, Interpreter interpreter, string commandLineArguments, bool isInteractive) {
            Session session;
            lock (_sessions) {
                var oldUserSessions = GetOrCreateSessionList();

                var oldSessions = oldUserSessions.Where(s => s.Id == id).ToArray();
                foreach (var oldSession in oldSessions) {
                    oldUserSessions.Remove(oldSession);
                    Task.Run(() => oldSession.KillHost()).SilenceException<Exception>().DoNotWait();
                    oldSession.State = SessionState.Terminated;
                }

                var userSessions = GetOrCreateSessionList();
                session = new Session(this, _processService, _applicationLifetime, _sessionLogger, _messageLogger, interpreter, id, commandLineArguments, isInteractive);
                session.StateChanged += Session_StateChanged;

                userSessions.Add(session);
            }

            session.StartHost(
                _loggingOptions.LogFolder,
                _loggingOptions.LogPackets || _loggingOptions.LogHostOutput ? LogVerbosity.Traffic : LogVerbosity.Minimal);

            return session;
        }

        private void Session_StateChanged(object sender, SessionStateChangedEventArgs e) {
            var session = (Session)sender;
            if (e.NewState == SessionState.Terminated) {
                lock (_sessions) {
                    var userSessions = GetOrCreateSessionList();
                    userSessions.Remove(session);
                }
            }
        }
    }
}
