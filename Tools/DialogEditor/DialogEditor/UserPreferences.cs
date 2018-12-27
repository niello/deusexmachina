using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Xml.Linq;
using DialogLogic;

namespace DialogDesigner
{
    public class UserPreferences:IDialogUserProperties
    {
        private readonly DialogObject _dialog;
        private readonly Dictionary<string, string> _defaultLinks = new Dictionary<string, string>();
        private XElement _xDialog;
        private XElement _xLinks;

        public string DialogScriptFile
        {
            get { return _dialog.Dialog.ScriptFile; }
            set { _dialog.Dialog.ScriptFile = value; }
        }

        public UserPreferences(DialogObject dialogInfo)
        {
            _dialog = dialogInfo;
        }

        public void PrepareForReading(XElement xDialog)
        {
            var xCharProperties = xDialog.Element("characterProperties");
            Dictionary<int, string> map = xCharProperties != null ? _dialog.Dialog.AttributeCollection.ReadXml(xCharProperties, new List<string> {"IsPlayer"}) : null;
            
            var xCharNode = xDialog.Element("characters");
            if(xCharNode!=null)
            {
                foreach(var xChar in xCharNode.Elements("character"))
                {
                    string name;
                    if(!xChar.TryGetAttribute("name",out name) || string.IsNullOrEmpty(name))
                        continue;

                    RegisterCharacter(name);
                    var ch = _dialog.Dialog.DialogCharacters.FirstOrDefault(c => c.Name == name);
                    ch.ReadProperties(xChar, id => map == null ? null : (!map.ContainsKey(id) ? null : map[id]));
                }
            }

            var xLinksNode = xDialog.Element("links");
            if(xLinksNode!=null)
            {
                foreach(var xLink in xLinksNode.Elements("link"))
                {
                    string from, to;
                    if (xLink.TryGetAttribute("from", out from) && !string.IsNullOrEmpty(from) && xLink.TryGetAttribute("to", out to) && !string.IsNullOrEmpty(to) && !_defaultLinks.ContainsKey(to))
                        _defaultLinks.Add(to, from);
                }
            }

            Debug.Assert(_dialog!=null && _dialog.Dialog!=null);
        }

        public void PrepareForWriting(XElement xDialog, DialogObject dlg)
        {
            _xDialog = xDialog;

            var xCharProps = _xDialog.Element("characterProperties");
            if(xCharProps==null)
            {
                xCharProps = new XElement("characterProperties");
                _xDialog.Add(xCharProps);
            }
            else
                xCharProps.RemoveAll();

            var map = dlg.Dialog.AttributeCollection.WriteXml(xCharProps);

            var xChars = _xDialog.AddOrClearChild("characters");

            foreach(var ch in dlg.Dialog.DialogCharacters)
            {
                var xCh = new XElement("character");
                xCh.Add(new XAttribute("name", ch.Name));
                ch.WriteProperties(xCh, propName => map[propName]);
                xChars.Add(xCh);
            }

            Debug.Assert(_dialog != null && _dialog.Dialog != null);
        }

        #region Implementation of IDialogUserProperties

        public string GetDefaultNodeLink(string nodeName)
        {
            string res;
            if(_defaultLinks.TryGetValue(nodeName,out res))
                return res;
            return null;
        }

        public void RegisterCharacter(string charName)
        {
            if (!_dialog.Dialog.ContainsCharacter(charName))
                _dialog.Dialog.AddCharacter(charName);
        }

        public void SetDefaultLink(string from, string to)
        {
            if(_xLinks==null)
            {
                _xLinks = _xDialog.Element("links");
                if(_xLinks==null)
                {
                    _xLinks = new XElement("links");
                    _xDialog.Add(_xLinks);
                }
            }

            var xLink = new XElement("link");
            xLink.Add(new XAttribute("from", from), new XAttribute("to", to));
            _xLinks.Add(xLink);
        }

        #endregion
    }
}
