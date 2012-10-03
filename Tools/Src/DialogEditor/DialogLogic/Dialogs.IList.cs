using System;
using System.Collections;
using System.Collections.Generic;

namespace DialogLogic
{
    partial class Dialogs : IList<DialogInfo>
    {
        private readonly List<DialogInfo> _dialogs = new List<DialogInfo>();

        #region Implementation of IEnumerable

        public IEnumerator<DialogInfo> GetEnumerator()
        {
            return _dialogs.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        #endregion

        #region Implementation of ICollection<DialogInfo>

        public void Add(DialogInfo item)
        {
            _dialogs.Add(item);
            if (item != null)
            {
                item.DialogInfoChanged -= OnDialogInfoChanged;
                item.DialogInfoChanged += OnDialogInfoChanged;
            }
            _hasChanges = true;
        }

        public void Clear()
        {
            if (_dialogs.Count > 0)
            {
                foreach(var item in _dialogs)
                {
                    if(item!=null)
                        item.DialogInfoChanged -= OnDialogInfoChanged;
                }
                _dialogs.Clear();
                _hasChanges = true;
            }
        }

        public bool Contains(DialogInfo item)
        {
            return _dialogs.Contains(item);
        }

        public void CopyTo(DialogInfo[] array, int arrayIndex)
        {
            _dialogs.CopyTo(array, arrayIndex);
        }

        public bool Remove(DialogInfo item)
        {
            bool res = _dialogs.Remove(item);
            if (res)
            {
                _hasChanges = true;
                if(item!=null)
                    item.DialogInfoChanged -= OnDialogInfoChanged;
            }
            return res;
        }

        public int Count
        {
            get { return _dialogs.Count; }
        }

        bool ICollection<DialogInfo>.IsReadOnly
        {
            get { return ((ICollection<DialogInfo>)_dialogs).IsReadOnly; }
        }

        #endregion

        #region Implementation of IList<DialogInfo>

        public int IndexOf(DialogInfo item)
        {
            return _dialogs.IndexOf(item);
        }

        public void Insert(int index, DialogInfo item)
        {
            _dialogs.Insert(index,item);
            item.DialogInfoChanged -= OnDialogInfoChanged;
            item.DialogInfoChanged += OnDialogInfoChanged;
            _hasChanges = true;
        }

        public void RemoveAt(int index)
        {
            var dlg = _dialogs[index];
            dlg.DialogInfoChanged -= OnDialogInfoChanged;

            _dialogs.RemoveAt(index);
            _hasChanges = true;
        }

        public DialogInfo this[int index]
        {
            get { return _dialogs[index]; }
            set
            {
                if(!ReferenceEquals(_dialogs[index],value))
                {
                    if (_dialogs[index] != null)
                        _dialogs[index].DialogInfoChanged -= OnDialogInfoChanged;

                    _dialogs[index] = value;
                    _hasChanges = true;
                    
                    if(_dialogs[index]!=null)
                    {
                        _dialogs[index].DialogInfoChanged -= OnDialogInfoChanged;
                        _dialogs[index].DialogInfoChanged += OnDialogInfoChanged;
                    }
                }
            }
        }

        #endregion

        private void OnDialogInfoChanged(object sender, EventArgs e)
        {
            _hasChanges = true;
        }
    }
}
