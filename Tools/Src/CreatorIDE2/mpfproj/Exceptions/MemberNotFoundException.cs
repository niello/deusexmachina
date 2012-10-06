using System;

namespace Microsoft.VisualStudio.Project
{
    public class MemberNotFoundException : ComSpecificException
    {
        private const int HRes = VSConstants.DISP_E_MEMBERNOTFOUND;

        public MemberNotFoundException():
            base(HRes)
        {}

        public MemberNotFoundException(string message) :
            base(HRes, message)
        {}

        public MemberNotFoundException(string message, Exception innerException) :
            base(HRes, message, innerException)
        {}
    }
}
