using System;

namespace HrdLib
{
    public class HrdContractException : Exception
    {
        public HrdContractException(string message) :
            base(message)
        {
        }

        public HrdContractException(string message, Exception innerException) :
            base(message, innerException)
        {
        }
    }
}
