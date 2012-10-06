using System;
using System.Runtime.Serialization;

namespace Microsoft.VisualStudio.Project
{
    public class ComSpecificException : Exception
    {
        public new int HResult { get { return base.HResult; } }

        public ComSpecificException(int hResult)
        {
            base.HResult = hResult;
        }

        public ComSpecificException(int hResult, string message) :
            base(message)
        {
            base.HResult = hResult;
        }

        public ComSpecificException(int hResult, string message, Exception innerException) :
            base(message, innerException)
        {
            base.HResult = hResult;
        }

        public ComSpecificException(int hResult, SerializationInfo info, StreamingContext context) :
            base(info, context)
        {
            base.HResult = hResult;
        }
    }
}
