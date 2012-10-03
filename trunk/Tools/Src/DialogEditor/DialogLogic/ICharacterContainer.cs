using System.Collections.Generic;

namespace DialogLogic
{
    public interface ICharacterContainer
    {
        CharacterAttributeCollection AttributeCollection { get; }

        ICharacterContainer ParentContainer { get; }

        List<DialogCharacter> GetCharacters();

        bool TryGetCharacter(string name, out DialogCharacter character);

        bool ContainsCharacter(string name);

        DialogCharacter AddCharacter(string name);
    }
}
