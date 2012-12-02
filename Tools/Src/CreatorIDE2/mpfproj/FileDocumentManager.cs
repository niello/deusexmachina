/// Copyright (c) Microsoft Corporation.  All rights reserved.

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Shell.Interop;
using IOleServiceProvider = Microsoft.VisualStudio.OLE.Interop.IServiceProvider;

namespace Microsoft.VisualStudio.Project
{
	/// <summary>
	/// This class handles opening, saving of file items in the hierarchy.
	/// </summary>
	[CLSCompliant(false)]
	public class FileDocumentManager : DocumentManager
	{
		#region ctors

		public FileDocumentManager(FileNode node)
			: base(node)
		{
		}
		#endregion

		#region overriden methods

		/// <summary>
		/// Open a file using the standard editor
		/// </summary>
		/// <param name="logicalView">In MultiView case determines view to be activated by IVsMultiViewDocumentView. For a list of logical view GUIDS, see constants starting with LOGVIEWID_ defined in NativeMethods class</param>
		/// <param name="docDataExisting">IntPtr to the IUnknown interface of the existing document data object</param>
		/// <param name="windowFrameAction">Determine the UI action on the document window</param>
		/// <returns>A reference to the window frame that is mapped to the file</returns>
		public IVsWindowFrame Open(Guid logicalView, IntPtr docDataExisting, WindowFrameShowAction windowFrameAction)
		{
			return Open(false, false,  logicalView, docDataExisting, windowFrameAction);
		}

        public sealed override int Open(ref Guid logicalView, IntPtr docDataExisting, out IVsWindowFrame windowFrame, WindowFrameShowAction windowFrameAction)
        {
            IVsWindowFrame frame;
            var res = ComHelper.WrapFunction(false, Open, logicalView, docDataExisting, windowFrameAction, out frame).
                Check(() => (HResult) (frame == null ? VSConstants.S_FALSE : VSConstants.S_OK));
            windowFrame = frame;
            return res;
        }

		public sealed override int OpenWithSpecific(uint editorFlags, ref Guid editorType, string physicalView, ref Guid logicalView, IntPtr docDataExisting, out IVsWindowFrame windowFrame, WindowFrameShowAction windowFrameAction)
		{
		    IVsWindowFrame frame;
		    var hRes =
		        ComHelper.WrapFunction(false, OpenWithSpecific, editorFlags, editorType, physicalView, logicalView,
		                               docDataExisting, windowFrameAction, out frame).Check(() => frame == null ? HResult.False : HResult.Ok);
		    windowFrame = frame;
		    return hRes;
		}

        /// <summary>
        /// Open a file with a specific editor
        /// </summary>
        /// <param name="editorFlags">Specifies actions to take when opening a specific editor. Possible editor flags are defined in the enumeration Microsoft.VisualStudio.Shell.Interop.__VSOSPEFLAGS</param>
        /// <param name="editorType">Unique identifier of the editor type</param>
        /// <param name="physicalView">Name of the physical view. If null, the environment calls MapLogicalView on the editor factory to determine the physical view that corresponds to the logical view. In this case, null does not specify the primary view, but rather indicates that you do not know which view corresponds to the logical view</param>
        /// <param name="logicalView">In MultiView case determines view to be activated by IVsMultiViewDocumentView. For a list of logical view GUIDS, see constants starting with LOGVIEWID_ defined in NativeMethods class</param>
        /// <param name="docDataExisting">IntPtr to the IUnknown interface of the existing document data object</param>
        /// <param name="windowFrameAction">Determine the UI action on the document window</param>
        /// <returns>A reference to the window frame that is mapped to the file</returns>
        public IVsWindowFrame OpenWithSpecific(uint editorFlags, Guid editorType, string physicalView, Guid logicalView, IntPtr docDataExisting, WindowFrameShowAction windowFrameAction)
        {
            return Open(false, false, editorFlags, editorType, physicalView, logicalView, docDataExisting, windowFrameAction);
        }

		#endregion

		#region public methods
		/// <summary>
		/// Open a file in a document window with a std editor
		/// </summary>
		/// <param name="newFile">Open the file as a new file</param>
		/// <param name="openWith">Use a dialog box to determine which editor to use</param>
		/// <param name="windowFrameAction">Determine the UI action on the document window</param>
		public void Open(bool newFile, bool openWith, WindowFrameShowAction windowFrameAction)
		{
		    Open(newFile, openWith, Guid.Empty, windowFrameAction);
		}

		/// <summary>
		/// Open a file in a document window with a std editor
		/// </summary>
		/// <param name="newFile">Open the file as a new file</param>
		/// <param name="openWith">Use a dialog box to determine which editor to use</param>
		/// <param name="logicalView">In MultiView case determines view to be activated by IVsMultiViewDocumentView. For a list of logical view GUIDS, see constants starting with LOGVIEWID_ defined in NativeMethods class</param>
		/// <param name="windowFrameAction">Determine the UI action on the document window</param>
        /// <returns>A reference to the window frame that is mapped to the file</returns>
        public IVsWindowFrame Open(bool newFile, bool openWith, Guid logicalView, WindowFrameShowAction windowFrameAction)
		{
            var rdt = Node.ProjectMgr.Site.GetService(typeof(SVsRunningDocumentTable)) as IVsRunningDocumentTable;
			Debug.Assert(rdt != null, " Could not get running document table from the services exposed by this project");
            if (rdt == null)
                throw new COMException();

			// First we see if someone else has opened the requested view of the file.
			var path = GetFullPathForDocument();
            var docData = IntPtr.Zero;
			
			try
			{
                const uint flags = (uint)_VSRDTFLAGS.RDT_NoLock;

			    uint docCookie, itemid;
			    IVsHierarchy ivsHierarchy;
			    ErrorHandler.ThrowOnFailure(rdt.FindAndLockDocument(flags, path, out ivsHierarchy, out itemid, out docData, out docCookie));
			    
			    return Open(newFile, openWith, logicalView, docData, windowFrameAction);
			}
			catch(COMException e)
			{
				Trace.WriteLine("Exception :" + e.Message);
			    throw;
			}
			finally
			{
				if(docData != IntPtr.Zero)
				{
					Marshal.Release(docData);
				}
			}
		}

		#endregion

		#region virtual methods
		/// <summary>
		/// Open a file in a document window
		/// </summary>
		/// <param name="newFile">Open the file as a new file</param>
		/// <param name="openWith">Use a dialog box to determine which editor to use</param>
		/// <param name="logicalView">In MultiView case determines view to be activated by IVsMultiViewDocumentView. For a list of logical view GUIDS, see constants starting with LOGVIEWID_ defined in NativeMethods class</param>
		/// <param name="docDataExisting">IntPtr to the IUnknown interface of the existing document data object</param>
		/// <param name="windowFrameAction">Determine the UI action on the document window</param>
        /// <returns>A reference to the window frame that is mapped to the file</returns>
        public virtual IVsWindowFrame Open(bool newFile, bool openWith, Guid logicalView, IntPtr docDataExisting, WindowFrameShowAction windowFrameAction)
		{
		    Guid editorID;
            if (openWith)
                editorID = Guid.Empty;
            else
            {
                var fileNode = Node as FileNode;
                if (fileNode == null)
                    throw new COMException();
                editorID = fileNode.DefaultEditorTypeID;
            }

		    return Open(newFile, openWith, 0, editorID, null, logicalView, docDataExisting, windowFrameAction);
		}

		#endregion

		#region helper methods

        private IVsWindowFrame Open(bool newFile, bool openWith, uint editorFlags, Guid editorType, string physicalView, Guid logicalView, IntPtr docDataExisting, WindowFrameShowAction windowFrameAction)
		{
			if(Node == null || Node.ProjectMgr == null || Node.ProjectMgr.IsClosed)
			{
			    throw new COMException();
			}

			Debug.Assert(Node != null, "No node has been initialized for the document manager");
			Debug.Assert(Node.ProjectMgr != null, "No project manager has been initialized for the document manager");
			Debug.Assert(Node is FileNode, "Node is not FileNode object");

			string caption = GetOwnerCaption();
			string fullPath = GetFullPathForDocument();

			// Make sure that the file is on disk before we open the editor and display message if not found
			if(!((FileNode)Node).IsFileOnDisk(true))
			{
				// Inform clients that we have an invalid item (wrong icon)
				Node.OnInvalidateItems(Node.Parent);

				// Bail since we are not able to open the item
				// Do not return an error code otherwise an internal error message is shown. The scenario for this operation
				// normally is already a reaction to a dialog box telling that the item has been removed.
			    return null; // VSConstants.S_FALSE;
			}

			var uiShellOpenDocument = Node.ProjectMgr.Site.GetService(typeof(SVsUIShellOpenDocument)) as IVsUIShellOpenDocument;
			var serviceProvider = Node.ProjectMgr.Site.GetService(typeof(IOleServiceProvider)) as IOleServiceProvider;

            IVsWindowFrame windowFrame = null;
            try
			{
				Node.ProjectMgr.OnOpenItem(fullPath);
				int hResult;

				if(openWith)
				{
				    hResult = uiShellOpenDocument.OpenStandardEditor((uint) __VSOSEFLAGS.OSE_UseOpenWithDialog, fullPath,
				                                                    ref logicalView, caption, Node.ProjectMgr, Node.ID,
				                                                    docDataExisting, serviceProvider, out windowFrame);
				}
				else
				{
					__VSOSEFLAGS openFlags = 0;
					if(newFile)
					{
						openFlags |= __VSOSEFLAGS.OSE_OpenAsNewFile;
					}

					//NOTE: we MUST pass the IVsProject in pVsUIHierarchy and the itemid
					// of the node being opened, otherwise the debugger doesn't work.
					if(editorType != Guid.Empty)
					{
					    hResult = uiShellOpenDocument.OpenSpecificEditor(editorFlags, fullPath, ref editorType, physicalView,
					                                                    ref logicalView, caption, Node.ProjectMgr, Node.ID,
					                                                    docDataExisting, serviceProvider, out windowFrame);
					}
					else
					{
						openFlags |= __VSOSEFLAGS.OSE_ChooseBestStdEditor;
					    hResult = uiShellOpenDocument.OpenStandardEditor((uint) openFlags, fullPath, ref logicalView, caption,
					                                                    Node.ProjectMgr, Node.ID, docDataExisting, serviceProvider,
					                                                    out windowFrame);
					}
				}

				if(hResult != VSConstants.S_OK && hResult != VSConstants.S_FALSE /*&& hResult != VSConstants.OLE_E_PROMPTSAVECANCELLED*/)
				{
					ErrorHandler.ThrowOnFailure(hResult);
				}

				if(windowFrame != null)
				{
					object var;

					if(newFile)
					{
						ErrorHandler.ThrowOnFailure(windowFrame.GetProperty((int)__VSFPROPID.VSFPROPID_DocData, out var));
						var persistDocData = (IVsPersistDocData)var;
						ErrorHandler.ThrowOnFailure(persistDocData.SetUntitledDocPath(fullPath));
					}

				    ErrorHandler.ThrowOnFailure(windowFrame.GetProperty((int)__VSFPROPID.VSFPROPID_DocCookie, out var));
					Node.DocCookie = (uint)(int)var;

					if(windowFrameAction == WindowFrameShowAction.Show)
					{
						ErrorHandler.ThrowOnFailure(windowFrame.Show());
					}
					else if(windowFrameAction == WindowFrameShowAction.ShowNoActivate)
					{
						ErrorHandler.ThrowOnFailure(windowFrame.ShowNoActivate());
					}
					else if(windowFrameAction == WindowFrameShowAction.Hide)
					{
						ErrorHandler.ThrowOnFailure(windowFrame.Hide());
					}
				}

			    return windowFrame;
			}
			catch(COMException e)
			{
				Trace.WriteLine("Exception e:" + e.Message);
				CloseWindowFrame(ref windowFrame);
			    throw;
			}
		}


		#endregion
	}
}
