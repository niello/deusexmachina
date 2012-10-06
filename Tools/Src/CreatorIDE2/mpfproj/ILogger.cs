using System;

namespace Microsoft.VisualStudio.Project
{
    public interface ILogger
    {
        void LogMessage(LogMessageLevel level, string message);

        void LogException(LogMessageLevel level, Exception exception, string message);
    }
}
