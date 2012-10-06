using System;
using System.Text;

namespace Microsoft.VisualStudio.Project
{
    public sealed class Logger: ILogger
    {
        public static readonly Logger Instance = new Logger();

        private volatile ILogger _innerLogger;

        private Logger()
        { }

        public static void SetLogger(ILogger logger)
        {
            var oldLogger = Instance._innerLogger as IDisposable;
            Instance._innerLogger = logger;
            LogInfoFormatted("{0} is registered as the default logger.", logger.GetType());

            if (oldLogger != null)
                oldLogger.Dispose();
        }

        void ILogger.LogMessage(LogMessageLevel level, string message)
        {
            var logger = _innerLogger;
            if (logger == null)
                return;

            logger.LogMessage(level, message);
        }

        void ILogger.LogException(LogMessageLevel level, Exception exception, string message)
        {
            var logger = _innerLogger;
            if (logger == null)
                return;

            logger.LogException(level, exception, message);
        }

        public static void LogMessage(LogMessageLevel level, string message)
        {
            ((ILogger)Instance).LogMessage(level, message);
        }

        public static void LogNotice(string message)
        {
            LogMessage(LogMessageLevel.Notice, message);
        }

        public static void LogInfo(string message)
        {
            LogMessage(LogMessageLevel.Info, message);
        }

        public static void LogWarning(string message)
        {
            LogMessage(LogMessageLevel.Warning, message);
        }

        public static void LogError(string message)
        {
            LogMessage(LogMessageLevel.Error, message);
        }

        public static void LogFatal(string message)
        {
            LogMessage(LogMessageLevel.Fatal, message);
        }

        public static void Trace(string message)
        {
            LogMessage(LogMessageLevel.Trace, message);
        }

        public static void LogMessageFormatted(LogMessageLevel level, string format, params object[] args)
        {
            LogMessage(level, SafeFormat(format, args));
        }

        public static void LogNoticeFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Notice, SafeFormat(format, args));
        }

        public static void LogInfoFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Info, SafeFormat(format, args));
        }

        public static void LogWarningFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Warning, SafeFormat(format, args));
        }

        public static void LogErrorFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Error, SafeFormat(format, args));
        }

        public static void LogFatalFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Fatal, SafeFormat(format, args));
        }

        public static void TraceFormatted(string format, params object[] args)
        {
            LogMessage(LogMessageLevel.Trace, SafeFormat(format, args));
        }

        public static void LogException(LogMessageLevel level, Exception exception, string message)
        {
            ((ILogger)Instance).LogException(level, exception, message);
        }

        public static void LogException(LogMessageLevel level, Exception exception)
        {
            LogException(level, exception, null);
        }

        public static void LogException(Exception exception)
        {
            LogException(exception, null);
        }

        public static void LogException(LogMessageLevel level, Exception exception, string format, params object[] args)
        {
            LogException(level, exception, SafeFormat(format, args));
        }

        public static void LogException(Exception exception, string format, params object[] args)
        {
            LogException(LogMessageLevel.Error, exception, SafeFormat(format, args));
        }

        public static void TraceException(Exception exception)
        {
            TraceException(exception, null);
        }

        public static void TraceException(Exception exception, string format, params object[] args)
        {
            LogException(LogMessageLevel.Trace, exception, format, args);
        }

        private static string SafeFormat(string format, params object[] args)
        {
            if (string.IsNullOrEmpty(format) || args.Length == 0)
                return format;

            try
            {
                return string.Format(format, args);
            }
            catch (Exception ex)
            {
                var sBuilder = new StringBuilder();
                sBuilder.AppendFormat("Formatting exception. Format string: '{0}' with {1} argument(s).", format,
                                      args.Length).AppendLine();

                for (int i = 0; i < args.Length; i++)
                {
                    string argStr;
                    try
                    {
                        argStr = args[i] == null ? "NULL" : args[i].ToString();
                    }
                    catch (Exception ex2)
                    {
                        argStr = ex2.ToString();
                    }
                    sBuilder.AppendFormat("Arguments[{0}]: {1}", i, argStr).AppendLine();
                }

                sBuilder.AppendFormat("Exception: {0}", ex.Message);

                return sBuilder.ToString();
            }
        }
    }
}
