using System;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using CreatorIDE.Core;
using CreatorIDE.Engine;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Props = CreatorIDE.Settings.Properties;

namespace CreatorIDE.Package
{
    /// <summary>
    /// This is the class that implements the package exposed by this assembly.
    ///
    /// The minimum requirement for a class to be considered a valid package for Visual Studio
    /// is to implement the IVsPackage interface and register itself with the shell.
    /// This package uses the helper classes defined inside the Managed Package Framework (MPF)
    /// to do it: it derives from the Package class that provides the implementation of the 
    /// IVsPackage interface and uses the registration attributes defined in the framework to 
    /// register itself and its components with the shell.
    /// </summary>
    // This attribute tells the registration utility (regpkg.exe) that this class needs
    // to be registered as package.
    [PackageRegistration(UseManagedResourcesOnly = true)]
    // A Visual Studio component can be registered under different regitry roots; for instance
    // when you debug your package you want to register it in the experimental hive. This
    // attribute specifies the registry root to use if no one is provided to regpkg.exe with
    // the /root switch.
    [DefaultRegistryRoot("Software\\Microsoft\\VisualStudio\\9.0")]
    // This attribute is used to register the informations needed to show the this package
    // in the Help/About dialog of Visual Studio.
    [InstalledProductRegistration(false, "#110", "#112", Props.Version, IconResourceID = 400)]
    // In order be loaded inside Visual Studio in a machine that has not the VS SDK installed, 
    // package needs to have a valid load key (it can be requested at 
    // http://msdn.microsoft.com/vstudio/extend/). This attributes tells the shell that this 
    // package has a load key embedded in its resources.
    [ProvideLoadKey("Standard", Props.Version, Props.Product, Props.Company, 1)]
    [ProvideProjectFactory(
        typeof(CideProjectFactory),
        CideProjectFactory.ProjectName,
        CideProjectFactory.ProjectName + " Files (*." + CideProjectFactory.ProjectExtension + ");*." + CideProjectFactory.ProjectExtension,
        CideProjectFactory.ProjectExtension,
        CideProjectFactory.ProjectExtension,
        CideProjectFactory.TemplatesDirectory)]
    [ProvideEditorFactory(typeof(LevelEditorFactory), 113)]
    [ProvideMenuResource(1000, 1)]
    [Guid(GuidString)]
    public sealed class CidePackage : ProjectPackage
    {
        public const string GuidString = "dcc26fbc-ef0a-4431-a4eb-84842197c818";

        private readonly Lazy<CideEngine> _engine = new Lazy<CideEngine>(() => new CideEngine());

        public CideEngine Engine { get { return _engine.Value; } }

        /// <summary>
        /// Default constructor of the package.
        /// Inside this method you can place any initialization code that does not require 
        /// any Visual Studio service because at this point the package object is created but 
        /// not sited yet inside Visual Studio environment. The place to do all the other 
        /// initialization is the Initialize method.
        /// </summary>
        public CidePackage()
        {
            Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "Entering constructor for: {0}", this.ToString()));
        }


        /////////////////////////////////////////////////////////////////////////////
        // Overriden Package Implementation
        #region Package Members

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initilaization code that rely on services provided by VisualStudio.
        /// </summary>
        protected override void Initialize()
        {
            Trace.WriteLine (string.Format(CultureInfo.CurrentCulture, "Entering Initialize() of: {0}", this.ToString()));
            base.Initialize();

            RegisterProjectFactory(new CideProjectFactory(this));

            RegisterEditorFactory(new LevelEditorFactory(this));
        }

        protected override bool TryCreateListener<TListener>(Func<ProjectPackage, TListener> factoryMethod, out TListener listener)
        {
            if (typeof(TListener) == typeof(SolutionListenerForProjectReferenceUpdate))
            {
                listener = null;
                return false;
            }
            return base.TryCreateListener(factoryMethod, out listener);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
                _engine.Dispose();

            base.Dispose(disposing);
        }

        #endregion
    }
}