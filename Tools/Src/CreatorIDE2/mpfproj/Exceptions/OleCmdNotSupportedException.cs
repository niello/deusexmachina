using System;
using System.Runtime.Serialization;
using Microsoft.VisualStudio.OLE.Interop;

namespace Microsoft.VisualStudio.Project.Exceptions
{
    public class OleCmdNotSupportedException:ComSpecificException
    {
        private const int HRes = (int) Constants.OLECMDERR_E_NOTSUPPORTED;

        public OleCmdNotSupportedException():
            base(HRes)
        {
        }

        public OleCmdNotSupportedException(string message):
            base(HRes, message)
        {
        }

        public OleCmdNotSupportedException(string message, Exception innerException):
            base(HRes, message, innerException)
        {
        }

        public OleCmdNotSupportedException(SerializationInfo info, StreamingContext context):
            base(HRes, info, context)
        {
        }
    }
}
