using CreatorIDE.Core;

namespace CreatorIDE
{
    internal sealed class WindowManager : IWindowManager
    {
        private readonly MainWnd _mainWnd;

        public WindowManager(MainWnd mainWnd)
        {
            _mainWnd = mainWnd;
        }
    }
}
