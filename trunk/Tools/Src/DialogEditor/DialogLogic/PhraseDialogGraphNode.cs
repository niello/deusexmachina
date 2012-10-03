namespace DialogLogic
{
    public class PhraseDialogGraphNode:DialogGraphPhraseNodeBase
    {
        public override DialogGraphNodeType NodeType
        {
            get { return DialogGraphNodeType.Phrase; }
        }
    }
}
