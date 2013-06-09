using System;

namespace HrdLib
{
    public class HrdStructureValidationException: Exception
    {
        public HrdStructureValidationException(string message) :
            base(message)
        {
        }
    }
}
