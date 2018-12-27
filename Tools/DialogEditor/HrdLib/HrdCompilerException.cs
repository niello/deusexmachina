using System;
using System.CodeDom.Compiler;

namespace HrdLib
{
    public class HrdCompilerException:Exception
    {
        private readonly CompilerErrorCollection _errors;

        public CompilerErrorCollection Errors { get { return _errors; } }

        public HrdCompilerException(string message, CompilerErrorCollection errors) :
            base(message)
        {
            if (errors == null)
                throw new ArgumentNullException("errors");
            if (errors.Count == 0)
                throw new ArgumentException(SR.GetString(SR.CompilerErrorCollectionEmpty), "errors");

            _errors = errors;
        }
    }
}
