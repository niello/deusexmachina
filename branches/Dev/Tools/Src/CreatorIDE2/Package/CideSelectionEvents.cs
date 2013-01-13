using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;
using CreatorIDE.Core;

namespace CreatorIDE.Package
{
    internal sealed class CideSelectionEvents : IVsSelectionEvents
    {
        private readonly Dictionary<uint, Guid> _contextMap = new Dictionary<uint, Guid>();
        private readonly Dictionary<Guid, bool> _stateMap = new Dictionary<Guid, bool>();
        private int _cookie;

        public CideSelectionEvents(CidePackage package, IEnumerable<Guid> uiContextIDs)
        {
            if (package == null)
                throw new ArgumentNullException("package");

            var monitor = package.GetService<IVsMonitorSelection>(typeof (SVsShellMonitorSelection));
            uint cookie;
            foreach(var cmdID in (uiContextIDs??Enumerable.Empty<Guid>()))
            {
                var c = cmdID;
                monitor.GetCmdUIContextCookie(ref c, out cookie);
                _contextMap.Add(cookie, cmdID);
            }

            monitor.AdviseSelectionEvents(this, out cookie);
            unchecked
            {
                _cookie = (int) cookie;
            }
        }

        int IVsSelectionEvents.OnSelectionChanged(IVsHierarchy pHierOld, uint itemidOld, IVsMultiItemSelect pMISOld, ISelectionContainer pSCOld, IVsHierarchy pHierNew, uint itemidNew, IVsMultiItemSelect pMISNew, ISelectionContainer pSCNew)
        {
            return HResult.Ok;
        }

        int IVsSelectionEvents.OnElementValueChanged(uint elementid, object varValueOld, object varValueNew)
        {
            return HResult.Ok;
        }

        int IVsSelectionEvents.OnCmdUIContextChanged(uint dwCmdUICookie, int fActive)
        {
            Guid id;
            if (_contextMap.TryGetValue(dwCmdUICookie, out id))
                _stateMap[id] = fActive != 0;
            return HResult.Ok;
        }

        public bool? IsActive(Guid uiContextID)
        {
            bool result;
            if (_stateMap.TryGetValue(uiContextID, out result))
                return result;

            return _contextMap.ContainsValue(uiContextID) ? false : (bool?) null;
        }

        public void Unadvise(CidePackage package)
        {
            if (package == null)
                throw new ArgumentNullException("package");

            uint cookie;
            unchecked
            {
                cookie = (uint) Interlocked.Exchange(ref _cookie, 0);
            }
            if (cookie == 0)
                return;

            var monitor = package.GetService<IVsMonitorSelection>(typeof(SVsShellMonitorSelection));
            monitor.UnadviseSelectionEvents(cookie);
        }
    }
}
