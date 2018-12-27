using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class ControlDialogNodeEditor : UserControl
    {
        private DialogObjectManager _manager;

        private DialogObject _dialog;
        public DialogObject Dialog
        {
            get { return _dialog; }
            set
            {
                Link = null;

                _dialog = value;
                SetNodeEditorEnabled(false);
                if (_dialog == null)
                    _bindingDialog.DataSource = typeof(DialogInfo);
                else
                {
                    var resetBinding = ReferenceEquals(_bindingDialog.DataSource, _dialog.Dialog);
                    _bindingDialog.DataSource = _dialog.Dialog;
                    if(resetBinding)
                        _bindingCharacters.ResetBindings(false);
                }
            }
        }

        private DialogGraphLink _link;
        public DialogGraphLink Link
        {
            get { return _link; }
            set
            {
                _link = value;

                DialogGraphNodeBase node=null;
                if (_dialog != null && _dialog.Dialog!=null && _link != null)
                    node = _link.Direction.ToNode == null ? null : _dialog.Dialog.Graph.GetNode(_link.Direction.ToNode.Value);

                if (_dialog == null || _dialog.Dialog==null || _link == null || !(node is DialogGraphPhraseNodeBase))
                {
                    SetNodeEditorEnabled(false);
                    _bindingDialogNode.DataSource = typeof (DialogGraphPhraseNodeBase);
                }
                else
                {
                    SetNodeEditorEnabled(node.DefaultLinkHere == _link);
                    _bindingDialogNode.DataSource = node;
                }

                _bindingLink.DataSource = (object) _link ?? typeof (DialogGraphLink);
            }
        }

        public ControlDialogNodeEditor()
        {
            InitializeComponent();

            SetNodeEditorEnabled(false);
        }

        public void Init(DialogObjectManager manager)
        {
            _manager = manager;
        }

        private void SetNodeEditorEnabled(bool enabled)
        {
            var enabledControlNames =
                (from ctrl in new Control[] {_buttonOpenScriptFile, _textCondition, _textAction} select ctrl.Name).ToList();
            foreach(Control ctrl in Controls)
            {
                if (ctrl is Label)
                    continue;
                if (_link != null && enabledControlNames.Contains(ctrl.Name))
                {
                    ctrl.Enabled = true;
                    continue;
                }

                ctrl.Enabled = enabled;
            }
        }

        private void ButtonOpenScriptFileClick(object sender, EventArgs e)
        {
            if(Dialog==null || Dialog.Dialog==null)
                return;

            try
            {
                var dlg = Dialog.Dialog;
                var script = string.IsNullOrEmpty(dlg.ScriptFile) ? Dialog.GetName() + ".lua" : dlg.ScriptFile;
                Debug.Assert(!string.IsNullOrEmpty(script));
                var fullPath = Dialog.GetFullPath(_manager, script);
                var dir = Path.GetDirectoryName(fullPath);
                Debug.Assert(!string.IsNullOrEmpty(dir));
                if (!Directory.Exists(dir))
                    Directory.CreateDirectory(dir);
                if (!File.Exists(fullPath))
                    using (File.CreateText(fullPath)) {}
                var proc = new Process {StartInfo = {FileName = fullPath, UseShellExecute = true}};
                proc.Start();
            }
            catch(Exception ex)
            {
                this.ShowError(ex);
            }
        }
        
    }
}
