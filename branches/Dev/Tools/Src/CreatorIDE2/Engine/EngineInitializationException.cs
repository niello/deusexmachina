using System;

namespace CreatorIDE.Engine
{
    public class EngineInitializationException: Exception
    {
        public EngineInitializationException(string message) :
            base(message)
        {
        }
    }
}
