using System;
using System.Diagnostics;
using System.IO;
using DialogLogic;

namespace DialogDesigner
{
    public class DialogObject
    {
        private DialogInfo _dialog;
        public DialogInfo Dialog
        {
            get { return _dialog; }
            set
            {
                if (_dialog != null)
                    _dialog.DialogInfoChanged -= OnDialogInfoChanges;
                if(value!=null)
                    value.DialogInfoChanged += OnDialogInfoChanges;
                _dialog = value;
                HasChanges = false;
            }
        }

        private string _relativePath;
        public string RelativePath
        {
            get { return _relativePath; }
            set
            {
                if (_relativePath != value)
                {
                    _relativePath = value;
                    OnPropertyChanged("RelativePath");
                }
            }
        }

        public bool HasChanges { get; set; }

        public string GetName()
        {
            int idx = RelativePath.LastIndexOf('\\');
            if(idx<0)
                return RelativePath;

            Debug.Assert(idx < RelativePath.Length - 1);
            return RelativePath.Substring(idx + 1);
        }

        public string GetRelativePath(DialogObjectManager manager, string fileName)
        {
            string directory;
            int idx = RelativePath.LastIndexOf('\\');
            if (idx <= 0)
                directory = manager.RootDirectory;
            else
                directory = Path.Combine(manager.RootDirectory, RelativePath.Substring(0, idx));

            return PathHelper.GetRelativePath(directory, fileName);
        }

        public string GetFullPath(DialogObjectManager manager, string relativePath)
        {
            string directory=null;
            int lastBackslash = RelativePath.IndexOf('\\');
            if (lastBackslash > 0)
                directory = RelativePath.Substring(0, lastBackslash);

            directory = string.IsNullOrEmpty(directory)
                            ? manager.RootDirectory
                            : Path.Combine(manager.RootDirectory, directory);

            return Path.GetFullPath(Path.Combine(directory, relativePath));
        }

        private void OnPropertyChanged(string propertyName)
        {
            HasChanges = true;
        }

        private void OnDialogInfoChanges(object sender, EventArgs e)
        {
            HasChanges = true;
        }
    }
}
