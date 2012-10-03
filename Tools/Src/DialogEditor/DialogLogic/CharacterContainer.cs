namespace DialogLogic
{
    public static class CharacterContainer
    {
        public static DialogCharacter GetOrCreateCharacter(this ICharacterContainer container, string name)
        {
            DialogCharacter result;
            if(container.TryGetCharacter(name,out result))
                return result;
            return container.AddCharacter(name);
        }
    }
}
