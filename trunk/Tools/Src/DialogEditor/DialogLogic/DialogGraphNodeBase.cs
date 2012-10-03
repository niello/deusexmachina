using System.ComponentModel;

namespace DialogLogic
{
    public abstract class DialogGraphNodeBase:IDisplayable,INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private int _id;
        public int Id
        {
            get { return _id; }
            internal set
            {
                if (_id != value)
                {
                    _id = value;
                    OnPropertyChanged("Id");
                }
            }
        }

        private DialogGraphLink _defaultLinkHere;
        public DialogGraphLink DefaultLinkHere
        {
            get { return _defaultLinkHere; }
            internal set
            {
                if (_defaultLinkHere != value)
                {
                    _defaultLinkHere = value;
                    OnPropertyChanged("DefaultLinkHere");
                }
            }
        }

        public abstract DialogGraphNodeType NodeType { get; }

        public abstract string DisplayName { get; }

        protected void OnPropertyChanged(string propertyName)
        {
            var eh = PropertyChanged;
            if (eh != null)
                eh(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
