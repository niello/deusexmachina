using System;
using System.Windows.Forms;
using CreatorIDE.Core;

namespace CreatorIDE
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            try
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);

                var context = new CreatorIDEContextManager();

                var mainWnd = new MainWnd();
                var wndMan = new WindowManager(mainWnd);
                context.RegisterContext<IWindowManager>(wndMan);
                context.RegisterContext<IAppConfiguration>(new AppConfiguration());

                AppContextManager.SetContextManager(context);

                Application.Run(new MainWnd());
            }
            finally
            {
                AppContextManager.Dispose();
            }
        }
    }
}
