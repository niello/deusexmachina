using System;
using System.Drawing;

namespace CreatorIDE.Engine
{
    public class EngineMouseClickEventArgs: EventArgs
    {
        private readonly Point _point;
        private readonly int _button;
        private readonly EMouseAction _action;

        public Point Point { get { return _point; } }

        public int Button { get { return _button; } }

        public EMouseAction Action { get { return _action; } }

        public bool Handled { get; set; }

        public EngineMouseClickEventArgs(int x, int y, int button, EMouseAction action)
        {
            _point = new Point(x, y);
            _button = button;
            _action = action;
        }
    }
}
