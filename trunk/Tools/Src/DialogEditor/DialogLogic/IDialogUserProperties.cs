namespace DialogLogic
{
    public interface IDialogUserProperties
    {
        string GetDefaultNodeLink(string nodeName);

        void RegisterCharacter(string charName);
        void SetDefaultLink(string from, string to);

        string DialogScriptFile { get; set; }
    }
}
