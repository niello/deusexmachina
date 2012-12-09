using System;

namespace CreatorIDE.Engine
{
    public class EnginePathRequestEventArgs: EventArgs
    {
        private readonly string _path;

        public string Path{get { return _path; }}

        public string NormalizedPath { get; set; }

        public bool Handled { get; set; }

        public EnginePathRequestEventArgs(string path)
        {
            _path = path;
        }
    }
}
