export interface LachlanWord {
  word: string;
  sentence: string;
}

export const LACHLAN_WORDS: readonly LachlanWord[] = [
  { word: "the", sentence: "The cat is soft." },
  { word: "see", sentence: "I can see the moon." },
  { word: "have", sentence: "I have a red hat." },
  { word: "their", sentence: "Their dog likes to run." },
  { word: "who", sentence: "Who is at the door?" },
  { word: "into", sentence: "The frog jumped into the pond." },
  { word: "people", sentence: "Many people came to the park." },
  { word: "very", sentence: "The pumpkin is very big." },
  { word: "be", sentence: "I will be kind." },
  { word: "your", sentence: "Your shoes are by the door." },
  { word: "were", sentence: "We were happy to play." },
  { word: "by", sentence: "The book is by my bed." },
  { word: "friend", sentence: "My friend can ride a bike." },
  { word: "pretty", sentence: "The flower is pretty." },
  { word: "above", sentence: "The bat flew above the tree." },
  { word: "and", sentence: "Ben and Mia made a fort." },
  { word: "me", sentence: "Please read with me." },
  { word: "want", sentence: "I want an orange pumpkin." },
  { word: "talk", sentence: "We can talk after lunch." },
  { word: "my", sentence: "My coat is warm." },
  { word: "because", sentence: "I smiled because I won." },
  { word: "nothing", sentence: "Nothing was in the box." },
  { word: "among", sentence: "A pumpkin sat among the leaves." },
  { word: "from", sentence: "This card is from Grandma." },
  { word: "no", sentence: "No bats were in the cave." },
  { word: "walk", sentence: "We walk to the pond." },
  { word: "one", sentence: "I found one red leaf." },
  { word: "woman", sentence: "The woman waved to us." },
  { word: "another", sentence: "May I have another apple?" },
  { word: "against", sentence: "The broom rests against the wall." },
  { word: "as", sentence: "I am as quick as a fox." },
  { word: "book", sentence: "This book is about a ghost." },
  { word: "goes", sentence: "Dad goes to work early." },
  { word: "would", sentence: "I would like to help." },
  { word: "two", sentence: "Two owls sat in the tree." },
  { word: "move", sentence: "Please move your chair." },
  { word: "mother", sentence: "My mother made warm soup." },
  { word: "always", sentence: "I always brush my teeth." },
  { word: "said", sentence: "She said hello to me." },
  { word: "are", sentence: "The apples are red." },
  { word: "says", sentence: "The sign says stop." },
  { word: "or", sentence: "Would you like milk or water?" },
  { word: "does", sentence: "What does this button do?" },
  { word: "both", sentence: "Both socks are purple." },
  { word: "brother", sentence: "My brother tells funny jokes." },
  { word: "almost", sentence: "I am almost ready." },
  { word: "to", sentence: "We went to the zoo." },
  { word: "was", sentence: "The night was cold." },
  { word: "she", sentence: "She has a green bag." },
  { word: "for", sentence: "This gift is for you." },
  { word: "many", sentence: "Many stars shine at night." },
  { word: "four", sentence: "Four ducks swam in the pond." },
  { word: "father", sentence: "My father reads with me." },
  { word: "do", sentence: "I can do my best." },
  { word: "you", sentence: "You are a good helper." },
  { word: "we", sentence: "We made a leaf pile." },
  { word: "there", sentence: "Put the pumpkin over there." },
  { word: "any", sentence: "Do you have any crayons?" },
  { word: "fourth", sentence: "I came in fourth place." },
  { word: "water", sentence: "Please water the plant." },
  { word: "of", sentence: "I ate a bowl of soup." },
  { word: "what", sentence: "What is in the bag?" },
  { word: "they", sentence: "They are playing outside." },
  { word: "where", sentence: "Where did my hat go?" },
  { word: "been", sentence: "I have been to that park." },
  { word: "forty", sentence: "The jar holds forty beans." },
  { word: "today", sentence: "We will carve a pumpkin today." }
] as const;

export const LACHLAN_PROGRESS_KEY = "wordpuzzle.lachlan.progress.v1";

export const buildAcceptedWords = (target: string, dictionary: readonly string[]) => {
  const normalized = target.toLowerCase();
  const counts = new Map<string, number>();
  for (const letter of normalized) counts.set(letter, (counts.get(letter) ?? 0) + 1);

  return new Set(
    [normalized, ...dictionary]
      .map((word) => word.toLowerCase())
      .filter((word) => {
        if (word.length < 2 || word.length > normalized.length) return false;
        const used = new Map<string, number>();
        for (const letter of word) {
          const next = (used.get(letter) ?? 0) + 1;
          if (next > (counts.get(letter) ?? 0)) return false;
          used.set(letter, next);
        }
        return true;
      })
  );
};

export const clampLachlanProgress = (value: unknown, total = LACHLAN_WORDS.length) => {
  const parsed = typeof value === "number" ? value : Number(value);
  if (!Number.isInteger(parsed) || parsed < 0) return 0;
  return Math.min(parsed, total);
};

export const loadLachlanProgress = () => {
  try {
    return clampLachlanProgress(localStorage.getItem(LACHLAN_PROGRESS_KEY));
  } catch {
    return 0;
  }
};

export const saveLachlanProgress = (index: number) => {
  try {
    localStorage.setItem(LACHLAN_PROGRESS_KEY, String(clampLachlanProgress(index)));
  } catch {
    // Progress remains available for this session when storage is unavailable.
  }
};
