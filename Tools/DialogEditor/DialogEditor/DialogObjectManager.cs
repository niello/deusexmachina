using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.Linq;
using DialogLogic;

namespace DialogDesigner
{
    public class DialogObjectManager
    {
        private Dialogs _dialogs;
        private readonly HashSet<string> _dialogsToRemove = new HashSet<string>();

        private readonly Dictionary<string, DialogObject> _directoryDialogMapping =
            new Dictionary<string, DialogObject>();

        private XElement _additionalData;

        private string _rootDirectory;
        public string RootDirectory
        {
            get { return _rootDirectory; }
            set
            {
                if (_rootDirectory == value)
                    return;
                _rootDirectory = value;
                OnRootDirectoryChanged();
            }
        }

        private string _currentXmlFile;
        public string XmlFile
        {
            get { return _currentXmlFile; }
            set
            {
                if(_currentXmlFile==value)
                    return;

                _currentXmlFile = value;
                OnXmlFileChanged();
            }
        }

        private bool _hasChanges;
        public bool HasChanges
        {
            get { return _hasChanges || (_dialogs != null && _dialogs.HasChanges); }
            set
            {
                _hasChanges = value;
                if (!_hasChanges)
                {
                    if (_dialogs != null)
                        _dialogs.HasChanges = false;
                    foreach (var dlgObj in _directoryDialogMapping.Values.Where(dlg => dlg != null))
                        dlgObj.HasChanges = false;
                }
            }
        }

        public Dialogs Dialogs { get { return _dialogs; } }

        public event Action RootDirectoryChanged;

        public DialogObjectManager()
        {
            _dialogs = new Dialogs();
        }

        public bool CheckDialogsForChanges()
        {
            return _directoryDialogMapping.Values.Any(val => val.HasChanges);
        }

        private void OnRootDirectoryChanged()
        {
            _directoryDialogMapping.Clear();
            _dialogs = new Dialogs();

            HasChanges = false;
            var action = RootDirectoryChanged;
            if (action != null)
                action();
        }

        private void LoadCharacters()
        {
            if(_additionalData==null)
                return;

            var xCharacters = _additionalData.Element("characters");
            if(xCharacters==null)
                return;

            var xCharProp = _additionalData.Element("characterProperties");
            Dictionary<int, string> map = xCharProp == null
                                              ? null
                                              : _dialogs.AttributeCollection.ReadXml(xCharProp,
                                                                                     new List<string>
                                                                                         {"DisplayName", "Name", "Id"});
            foreach(var xChar in xCharacters.Elements("character"))
            {
                string name;
                if(!xChar.TryGetAttribute("name",out name) || string.IsNullOrEmpty(name))
                    continue;

                var ch = _dialogs.GetOrCreateCharacter(name);
                ch.ReadProperties(xChar, id => map == null ? null : (map.ContainsKey(id) ? map[id] : null));
            }
        }

        public DialogObject GetOrCreateDialogObject(string relativePath)
        {
            string relPath = relativePath.ToLower();

            DialogObject result;
            if(_directoryDialogMapping.TryGetValue(relPath,out result))
                return result;

            result=new DialogObject();
            string fPath = null;
            if (RootDirectory != null)
                fPath = Path.Combine(RootDirectory, relPath + ".dlg");

            var changed = _dialogs.HasChanges;

            var dlgInfo = new DialogInfo(_dialogs);
            result.Dialog = dlgInfo;
            if (File.Exists(fPath))
            {
                var pref = new UserPreferences(result);
                var xDialog = GetDialogNode(relativePath);
                pref.PrepareForReading(xDialog ?? new XElement("dialog", new XAttribute("path", relativePath)));
                dlgInfo.LoadFromFile(fPath, pref);
            }
            result.RelativePath = relativePath;
            result.HasChanges = false;
            _dialogs.Add(dlgInfo);

            if(!changed)
                _dialogs.HasChanges = false;

            _directoryDialogMapping.Add(relPath, result);
            return result;
        }

        public void Reset()
        {
            XmlFile = null;
            RootDirectory = null;
        }

        public void SaveChanges(string newXmlFilePath, string rootDirectory, List<string> list)
        {
            if(_additionalData==null)
                _additionalData = new XElement("additionalData");

            var xCfg = _additionalData.Element("configuration");
            if(xCfg==null)
            {
                xCfg = new XElement("configuration");
                _additionalData.Add(xCfg);
            }

            var xRootDir = xCfg.Element("rootDirectory");
            if(xRootDir==null)
            {
                xRootDir=new XElement("rootDirectory");
                xCfg.Add(xRootDir);
            }

            string rootDirRelative = string.Empty;
            if(!string.IsNullOrEmpty(RootDirectory))
            {
                var newDir = Path.GetFullPath(Path.GetDirectoryName(newXmlFilePath));
                rootDirRelative = PathHelper.GetRelativePath(newDir, RootDirectory);
            }
            xRootDir.Value = rootDirRelative;

            var xCharacters = _additionalData.Element("characters");
            if(xCharacters!=null)
                xCharacters.Remove();

            xCharacters = new XElement("characters");
            var xProperties = new XElement("characterProperties");
            var map = _dialogs.AttributeCollection.WriteXml(xProperties);
            xCharacters.Add(xProperties);
            
            var tmp = new XElement("characters");
            xCharacters.Add(tmp);
            xCharacters = tmp;

            foreach (var ch in _dialogs.Characters.Values)
            {
                var xCh = new XElement("character");
                xCh.Add(new XAttribute("name", ch.Name));
                ch.WriteProperties(xCh, id => map[id]);
                xCharacters.Add(xCh);
            }

            _additionalData.Add(xCharacters);

            var xDialogs = _additionalData.Element("dialogs");
            if(xDialogs==null)
            {
                xDialogs = new XElement("dialogs");
                _additionalData.Add(xDialogs);
            }

            var dlgDict = (from xD in xDialogs.Elements("dialog")
                           let path = xD.Attribute("path")
                           where path != null && !string.IsNullOrEmpty(path.Value)
                           select new {Key = path.Value, Value = xD}).ToDictionary(pair => pair.Key, pair => pair.Value);

            xDialogs.RemoveAll();

            var ignoredDialogs = new HashSet<string>();
            if (list != null)
            {
                foreach (var key in _directoryDialogMapping.Keys)
                    ignoredDialogs.Add(key);
                foreach (var str in list.Select(s => s.ToLower()))
                    ignoredDialogs.Remove(str);
            }

            foreach(var pair in _directoryDialogMapping)
            {
                if(ignoredDialogs.Contains(pair.Key))
                    continue;

                XElement xDlg;
                if (!dlgDict.TryGetValue(pair.Key, out xDlg))
                    xDlg = new XElement("dialog");
                else
                    dlgDict.Remove(pair.Key);

                var dlg = pair.Value;
                if(dlg.HasChanges)
                {
                    xDlg.RemoveAll();
                    xDlg.Add(new XAttribute("path", pair.Key));
                    var pref = new UserPreferences(dlg);
                    var fPath = Path.Combine(rootDirectory, dlg.RelativePath + ".dlg");
                    var dirPath = Path.GetDirectoryName(fPath);
                    if (!Directory.Exists(dirPath))
                        Directory.CreateDirectory(dirPath);

                    pref.PrepareForWriting(xDlg, dlg);
                    dlg.Dialog.SaveToFile(fPath, pref);
                }

                xDialogs.Add(xDlg);
            }

            foreach(var pair in dlgDict)
            {
                string fullPath = Path.Combine(xRootDir.Value, pair.Key + ".dlg");
                if(File.Exists(fullPath))
                {
                    if (_dialogsToRemove.Contains(pair.Key))
                    {
                        try
                        {
                            File.Delete(fullPath);
                            _dialogsToRemove.Remove(pair.Key);
                        }
                        catch
                        {
                            //Nobody cares
                            Console.WriteLine(@"Unable to delete file '{0}'", fullPath);
                        }
                    }
                    else
                        xDialogs.Add(pair.Value);
                }
            }

            foreach(var dlgToRemove in _dialogsToRemove)
            {
                string path = Path.Combine(xRootDir.Value, dlgToRemove + ".dlg");
                if(!File.Exists(path))
                    continue;
                try
                {
                    File.Delete(path);
                }
                catch
                {
                    //Nobody cares
                    Console.WriteLine(@"Unable to delete file '{0}'", path);
                }
            }

            _dialogsToRemove.Clear();

            using (var fStream=new FileStream(newXmlFilePath,FileMode.Create,FileAccess.Write))
            {
                using(var writer=XmlWriter.Create(fStream,new XmlWriterSettings{Indent=true}))
                {
                    var xDoc = new XDocument(_additionalData);
                    xDoc.WriteTo(writer);
                }
            }

            XmlFile = newXmlFilePath;
            RootDirectory = rootDirectory;
            HasChanges = false;
        }

        private XElement GetDialogNode(string path)
        {
            if(_additionalData!=null)
            {
                var xDialogs = _additionalData.Element("dialogs");
                if (xDialogs != null)
                    return xDialogs.Elements("dialog").FirstOrDefault(xDlg =>
                                                                          {
                                                                              string p;
                                                                              return
                                                                                  xDlg.TryGetAttribute("path", out p) &&
                                                                                  p == path;
                                                                          });
            }
            return null;
        }

        private void OnXmlFileChanged()
        {
            if (XmlFile == null || !File.Exists(XmlFile))
            {
                RootDirectory = null;
                _additionalData = null;
            }
            else
            {
                var doc = XDocument.Load(XmlFile);

                _additionalData = doc.Element("additionalData");
                if (_additionalData != null)
                {
                    LoadCharacters();
                    var xCfg = _additionalData.Element("configuration");
                    if (xCfg != null)
                    {
                        var xRootDir = xCfg.Element("rootDirectory");
                        if (xRootDir != null && !string.IsNullOrEmpty(xRootDir.Value))
                        {
                            var rootDir = xRootDir.Value;
                            if(!Path.IsPathRooted(rootDir))
                            {
                                var baseDir = Path.GetFullPath(Path.GetDirectoryName(XmlFile));
                                rootDir = Path.GetFullPath(Path.Combine(baseDir, rootDir));
                            }

                            RootDirectory = Directory.Exists(rootDir) ? rootDir : null;
                        }
                        else
                            RootDirectory = null;
                    }
                }
            }
        }

        public void RenameFolder(string relativePath, string newFolderName)
        {
            var path = relativePath.ToLower() + '\\';
            List<string> toRename =
                _directoryDialogMapping.Where(pair => pair.Key.StartsWith(path)).Select(pair => pair.Key).ToList();

            int lastBackslash = relativePath.LastIndexOf('\\');
            string left = (lastBackslash > 0 ? relativePath.Substring(0, lastBackslash + 1) : string.Empty) + newFolderName;
            foreach(var dlgPath in toRename)
            {
                var dObj = _directoryDialogMapping[dlgPath];
                string right = dObj.RelativePath.Substring(path.Length - 1);
                dObj.RelativePath = left + right;
                _directoryDialogMapping.Remove(dlgPath);
                _directoryDialogMapping.Add(dObj.RelativePath.ToLower(), dObj);
            }

            string fullPath = Path.Combine(RootDirectory, relativePath), newFullPath = Path.Combine(RootDirectory, left);
            RenameRealFolder(fullPath, newFullPath);
            _hasChanges = true;
        }

        private static void RenameRealFolder(string folderPath, string newFolderPath)
        {
            if(!Directory.Exists(folderPath))
                return;

            Directory.Move(folderPath, newFolderPath);
        }

        public void RenameDialog(string relativePath, string newDialogName)
        {
            var path = relativePath.ToLower();
            DialogObject info;
            if(!_directoryDialogMapping.TryGetValue(path,out info))
                return;

            int lastBackslash = relativePath.LastIndexOf('\\');
            string newPath = (lastBackslash > 0 ? relativePath.Substring(0, lastBackslash + 1) : string.Empty) +
                             newDialogName;
            info.RelativePath = newPath;
            _directoryDialogMapping.Remove(path);
            path = newPath.ToLower();
            _directoryDialogMapping.Add(path, info);
            _hasChanges = true;
        }

        public void RemoveFolder(string relativePath)
        {
            var path = relativePath.ToLower() + '\\';
            List<string> toRemove = _directoryDialogMapping.Keys.Where(key => key.StartsWith(path)).ToList();

            foreach (var key in toRemove)
                RemoveDialogInternal(key);

            var fullPath = Path.Combine(RootDirectory, relativePath);
            RemoveRealFolder(fullPath);
            _hasChanges = true;
        }

        private static void RemoveRealFolder(string fullPath)
        {
            if(Directory.Exists(fullPath))
            {
                var info = new DirectoryInfo(fullPath);
                if (ContainsOnlyDialogs(info))
                    Directory.Delete(fullPath, true);
            }
        }

        private static bool ContainsOnlyDialogs(DirectoryInfo info)
        {
            foreach(var fInfo in info.GetFiles())
            {
                if(fInfo.Extension!=".dlg")
                    return false;
            }

            foreach(var dirInfo in info.GetDirectories())
            {
                // ignore svn folder
                if((dirInfo.Attributes & FileAttributes.Hidden)!=0)
                    continue;
                
                if(!ContainsOnlyDialogs(dirInfo))
                    return false;
            }

            return true;
        }

        public void RemoveDialog(string relativePath)
        {
            var path = relativePath.ToLower();
            RemoveDialogInternal(path);
            _hasChanges = true;
        }

        private void RemoveDialogInternal(string key)
        {
            DialogObject dlg;
            _directoryDialogMapping.TryGetValue(key, out dlg);
            _dialogsToRemove.Add(key);

            if(dlg==null)
                return;

            _dialogs.Remove(dlg.Dialog);
            _directoryDialogMapping.Remove(key);
            return;
        }

        public List<string> GetChangedFiles()
        {
            return
                (from dlgObj in _directoryDialogMapping.Values where dlgObj.HasChanges select dlgObj.RelativePath).
                    ToList();
        }
    }
}
