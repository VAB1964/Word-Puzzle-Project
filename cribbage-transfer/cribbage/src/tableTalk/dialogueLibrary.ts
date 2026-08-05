import type { CharacterId, TableTalkEmotion } from "./types";

export type DialogueKey =
  | "game_started"
  | "round_started"
  | "first_crib_won"
  | "self_fifteen"
  | "opp_fifteen"
  | "self_thirty_one"
  | "opp_thirty_one"
  | "self_pair"
  | "opp_pair"
  | "self_pair_royal"
  | "opp_pair_royal"
  | "self_double_pair_royal"
  | "opp_double_pair_royal"
  | "self_pegging_run"
  | "opp_pegging_run"
  | "go_declared"
  | "self_last_card"
  | "opp_last_card"
  | "self_large_hand"
  | "opp_large_hand"
  | "self_zero_hand"
  | "opp_zero_hand"
  | "self_large_crib"
  | "opp_large_crib"
  | "lead_changed_self"
  | "lead_changed_opp"
  | "close_to_winning_self"
  | "opponent_close_to_winning"
  | "falls_behind"
  | "catches_up"
  | "game_won"
  | "game_lost"
  | "generic_card_play";

type CharacterDialoguePools = Partial<
  Record<DialogueKey, { emotion: TableTalkEmotion; lines: string[] }>
>;

export const DIALOGUE_LIBRARY: Record<CharacterId, CharacterDialoguePools> = {
  mabel: {
    game_started: { emotion: "supportive", lines: ["Cards are on the table, everyone play nice.", "New game, fresh luck, and maybe my best shuffling face.", "All right, friends, let's make this a good one.", "Settle in, this should be a nice game.", "Here we are again, cards and good company."] },
    round_started: { emotion: "playful", lines: ["New round, new mischief.", "Let's see what this deal has in store.", "Round's live; keep your pegs warm.", "All right, let's see these cards.", "Maybe this deal will be kind to us."] },
    first_crib_won: { emotion: "playful", lines: ["My crib.", "My crib, send me good cards.", "Looks like I get the crib."] },
    self_fifteen: { emotion: "playful", lines: ["Fifteen for two, and I will absolutely take it.", "That fifteen came right on time.", "A tidy little fifteen.", "There we are, fifteen for two.", "That worked out rather nicely."] },
    opp_fifteen: { emotion: "supportive", lines: ["Nice fifteen there.", "Well played, that fifteen was clean.", "Good eye, that made fifteen nicely.", "You found a good fifteen there.", "That was a handy two points."] },
    self_thirty_one: { emotion: "competitive", lines: ["Thirty-one feels downright cozy.", "I'll gladly park on thirty-one.", "Thirty-one exactly; thank you kindly.", "Right on thirty-one, that will do.", "That landed just where I wanted it."] },
    opp_thirty_one: { emotion: "supportive", lines: ["Thirty-one on the nose, nice work.", "Perfect thirty-one. Couldn't argue with it.", "Sharp play; that thirty-one was tidy.", "Nicely placed on thirty-one.", "You timed that one well."] },
    self_pair: { emotion: "playful", lines: ["A quick pair keeps me smiling.", "Pair for me; little victories count.", "I'll borrow those two points, thanks.", "A pair will do just fine.", "Two quiet points for me."] },
    opp_pair: { emotion: "supportive", lines: ["That's a neat little pair.", "Pair well played.", "Couldn't miss that pair.", "A nice pair for you.", "That pair fit in nicely."] },
    self_pair_royal: { emotion: "competitive", lines: ["Pair royal! That'll wake the pegs up.", "Six points and a happy grin from me.", "Pair royal does brighten a hand.", "Six points is a pleasant turn.", "That pair royal came along nicely."] },
    opp_pair_royal: { emotion: "supportive", lines: ["Pair royal, very nice.", "Six points is a proper move.", "Now that's a lively pair royal.", "A good six points for you.", "That pair royal worked out well."] },
    self_double_pair_royal: { emotion: "competitive", lines: ["Twelve points? Mercy, that's lively.", "Double pair royal, and I won't pretend to be humble.", "Well now, twelve points does sparkle.", "That is a very welcome twelve.", "Well, those pegs can stretch their legs."] },
    opp_double_pair_royal: { emotion: "supportive", lines: ["Double pair royal, goodness me.", "Twelve points? Nicely done.", "That's a beautiful twelve-point burst.", "That was quite a turn for you.", "A very tidy twelve points."] },
    self_pegging_run: { emotion: "competitive", lines: ["That run marched in exactly when I needed it.", "A run for me; I'll call that tidy.", "Runs like that keep me cheerful.", "That run came together nicely.", "I had a good spot for that one."] },
    opp_pegging_run: { emotion: "supportive", lines: ["Lovely run there.", "That run was nicely spotted.", "Well found, that's a clean run.", "You put that run together well.", "A good little run for you."] },
    go_declared: { emotion: "playful", lines: ["Go."] },
    self_last_card: { emotion: "competitive", lines: ["Last card's mine, and I'll keep it.", "Last card point sneaks in nicely.", "I'll take that last-card point, thank you.", "One more point before we reset.", "That last card found a good home."] },
    opp_last_card: { emotion: "supportive", lines: ["Last card to you, nicely timed.", "You tucked away that last card point.", "Good finish to the sequence.", "You can have the last one.", "A quiet point to finish things off."] },
    self_large_hand: { emotion: "competitive", lines: ["Now that's a hand worth bragging about.", "Big hand for me; I'll try to act surprised.", "Those points came in with a smile.", "That hand treated me rather well.", "I cannot complain about that count."] },
    opp_large_hand: { emotion: "supportive", lines: ["That's a strong hand, nicely counted.", "Big points there; well earned.", "You found a beauty of a hand.", "That hand came together well.", "A very good count for you."] },
    self_zero_hand: { emotion: "playful", lines: ["Zero points. This hand is grounded.", "Well, that hand packed no lunch.", "Nothing there. We'll call it character building.", "Not much hiding in that hand.", "Well, we can move along from that one."] },
    opp_zero_hand: { emotion: "supportive", lines: ["Zero happens to all of us sometimes.", "Tough hand; next one will be kinder.", "No points there, but better cards are coming.", "That hand did not give you much.", "The next deal may suit you better."] },
    self_large_crib: { emotion: "competitive", lines: ["That's a hearty crib, thank you very much.", "Crib came through for me in style.", "Big crib points always feel extra sweet.", "That crib was worth waiting for.", "A generous little crib for me."] },
    opp_large_crib: { emotion: "supportive", lines: ["That's a healthy crib.", "Big crib there; fair play.", "Your crib paid off nicely.", "That crib worked out well for you.", "A good return from the crib."] },
    lead_changed_self: { emotion: "competitive", lines: ["Looks like I've slipped into the lead.", "Lead change in my favor; I'll keep moving.", "I found the front rail for now.", "I have edged ahead for the moment.", "Looks like my peg found the front."] },
    lead_changed_opp: { emotion: "supportive", lines: ["Lead's yours at the moment, nicely done.", "You've taken the lead, and cleanly too.", "Lead change your way; good pressure.", "You have moved out in front.", "Your peg has the better view now."] },
    close_to_winning_self: { emotion: "competitive", lines: ["I'm close, but I'm not celebrating yet.", "Getting near the finish peg now.", "Almost home, if luck behaves.", "Just a little farther to go.", "I am getting close, nice and steady."] },
    opponent_close_to_winning: { emotion: "concerned", lines: ["You're getting close to the finish line.", "You're near the end peg; message received.", "Close game now. We'll need a careful hand.", "You are getting a bit close for comfort.", "I see that peg nearing home."] },
    falls_behind: { emotion: "concerned", lines: ["I'm trailing a bit; time for steadier cards.", "That gap opened up on me.", "Behind for now, but not out.", "I have some ground to make up.", "The pegs are asking me to catch up."] },
    catches_up: { emotion: "optimistic", lines: ["Back in striking distance now.", "That closes the gap nicely.", "Caught up a fair bit; game on.", "There, that feels a little closer.", "I am finding my way back."] },
    game_won: { emotion: "supportive", lines: ["Good game, everyone. That was fun.", "Made it to 121. Great table, friends.", "I'll take the win and the good company.", "That was a lovely game, thank you.", "A nice finish to a good game."] },
    game_lost: { emotion: "supportive", lines: ["Well played. You earned that one.", "Good game, truly. I'll get you next hand.", "You got me this time; nicely done.", "That was well played from start to finish.", "A good game, and a fair win."] },
    generic_card_play: { emotion: "playful", lines: ["Interesting play there.", "That card changed the mood a little.", "All right, now we're talking.", "Let us see where that card leads.", "That gives us something to think about."] },
  },
  arthur: {
    game_started: { emotion: "dry", lines: ["Another game. My confidence has not been consulted.", "Cards ready. Dignity may be optional.", "Excellent, a fresh chance to misread everything.", "All right, let us see how this goes.", "Another game seems entirely reasonable."] },
    round_started: { emotion: "dry", lines: ["New round. I remain cautiously skeptical.", "Round begins. Expectations suitably moderate.", "Let's proceed with measured optimism.", "Fresh cards, same quiet confidence.", "We begin again, as tradition demands."] },
    first_crib_won: { emotion: "dry", lines: ["My crib.", "My crib. Try to be unhelpful, cards.", "Looks like I get the crib."] },
    self_fifteen: { emotion: "dry", lines: ["Fifteen for two. A modest miracle.", "I'll accept those two points quietly.", "Two points. A brief triumph.", "Fifteen. That will do nicely.", "A useful two points, no fuss."] },
    opp_fifteen: { emotion: "competitive", lines: ["A fine fifteen. Inconvenient for me.", "Nicely found. I noticed, unfortunately.", "Clean fifteen. Regrettably accurate.", "A sensible fifteen on your part.", "Two points placed rather well."] },
    self_thirty_one: { emotion: "competitive", lines: ["Thirty-one exactly. I'll frame that.", "At last, precision in my favor.", "Thirty-one. For once, the math cooperated.", "Thirty-one. Nicely settled.", "That count ended where it should."] },
    opp_thirty_one: { emotion: "dry", lines: ["Thirty-one. Disturbingly efficient.", "Perfectly placed. I object on principle.", "That was annoyingly precise.", "Right on thirty-one. Fair enough.", "A precise ending from you."] },
    self_pair: { emotion: "dry", lines: ["A pair. Tiny, but respectable.", "Two points. Better than a lecture.", "I'll bank the pair and move on.", "A pair. Nothing wrong with that.", "Two points without unnecessary ceremony."] },
    opp_pair: { emotion: "competitive", lines: ["Pair played well.", "That pair was a practical decision.", "A tidy pair. I can't deny it.", "A straightforward pair for you.", "That pair was well placed."] },
    self_pair_royal: { emotion: "competitive", lines: ["Pair royal. Finally, a card agrees with me.", "Six points. I may pretend this was planned.", "Pair royal and a brief surge of competence.", "Six points. A useful development.", "That pair royal suits me fine."] },
    opp_pair_royal: { emotion: "dry", lines: ["Pair royal. Bold and effective.", "Six points to you. Painfully efficient.", "That's a very real pair royal.", "A solid six points for you.", "That pair royal was hard to miss."] },
    self_double_pair_royal: { emotion: "competitive", lines: ["Twelve points. I would like this documented.", "Double pair royal. Astonishing.", "A rare moment where luck and I shook hands.", "Twelve points. I can work with that.", "That turned out rather well for me."] },
    opp_double_pair_royal: { emotion: "dry", lines: ["Double pair royal. Well, that escalated.", "Twelve points. Suboptimal for my plans.", "Impressive blast of points there.", "A calm twelve points for you.", "That was inconveniently effective."] },
    self_pegging_run: { emotion: "competitive", lines: ["A run appears and I gratefully accept.", "Run scored. Miracles continue.", "That run landed better than expected.", "A useful run at the right time.", "That sequence worked in my favor."] },
    opp_pegging_run: { emotion: "dry", lines: ["A lovely run. Rude, but lovely.", "Run well spotted.", "I saw it one second too late.", "A good run. I will allow it.", "You found the sequence cleanly."] },
    go_declared: { emotion: "self_deprecating", lines: ["Go."] },
    self_last_card: { emotion: "competitive", lines: ["Last card point secured with minimal drama.", "I'll take the final point.", "Last card. Efficient and mildly satisfying.", "One last point for the road.", "A quiet finish in my favor."] },
    opp_last_card: { emotion: "dry", lines: ["Last card to you. Cleanly done.", "You took the end point.", "Nicely closed out there.", "The final point is yours.", "You finished that sequence neatly."] },
    self_large_hand: { emotion: "dry", lines: ["A large hand. I promise not to gloat loudly.", "Those points were almost suspiciously generous.", "Substantial hand. I'll allow myself one smile.", "That count was better than expected.", "A strong hand. I have no complaints."] },
    opp_large_hand: { emotion: "competitive", lines: ["Strong hand. Very strong, unfortunately.", "Excellent count. I grudgingly admire it.", "Big points there. Nicely played.", "That was a respectable hand.", "A good count for you, clearly."] },
    self_zero_hand: { emotion: "self_deprecating", lines: ["Zero points. Consistency at last.", "Nothing. My hand has gone on strike.", "A perfectly empty hand count. Remarkable.", "Nothing there. Very economical.", "That hand contributed mainly atmosphere."] },
    opp_zero_hand: { emotion: "dry", lines: ["Zero points can happen to anyone.", "Tough draw. Seen it myself, often.", "No score there. Better luck next deal.", "That hand offered very little.", "The cards were not especially helpful."] },
    self_large_crib: { emotion: "competitive", lines: ["The crib has finally written me a love letter.", "Generous crib. I'll take the compliment.", "Crib points arrived in proper volume.", "A solid crib. Much appreciated.", "That crib quietly did its job."] },
    opp_large_crib: { emotion: "dry", lines: ["Healthy crib. Distressing, but fair.", "Your crib paid very well.", "A profitable crib indeed.", "That crib treated you well.", "A rather useful crib for you."] },
    lead_changed_self: { emotion: "competitive", lines: ["I've wandered into the lead.", "Lead change my way, temporarily I assume.", "I appear to be ahead for the moment.", "I seem to have moved in front.", "The lead is mine, for now."] },
    lead_changed_opp: { emotion: "dry", lines: ["Lead is yours. I noticed immediately.", "You've taken the front peg.", "Lead change against me. How original.", "You have edged ahead.", "Your peg is setting the pace now."] },
    close_to_winning_self: { emotion: "competitive", lines: ["Near the finish peg now. No mistakes.", "Close to the line, somehow.", "Approaching 121 with mild disbelief.", "Not far to go now.", "The finish is looking reasonably close."] },
    opponent_close_to_winning: { emotion: "concerned", lines: ["You're close now. I should probably panic politely.", "Near the finish, are we.", "You're within reach of the end peg.", "You are getting rather close.", "I have noticed your peg near the end."] },
    falls_behind: { emotion: "self_deprecating", lines: ["I've drifted behind. Tradition continues.", "Gap's opening up. Not ideal.", "Behind again. I blame my dramatic timing.", "I have given myself some work to do.", "The gap is becoming difficult to ignore."] },
    catches_up: { emotion: "competitive", lines: ["Back within range. That's better.", "I've closed the gap somewhat.", "Now we're back in the conversation.", "That puts me closer to things.", "I am becoming relevant again."] },
    game_won: { emotion: "competitive", lines: ["121 reached. Pleasant surprise.", "Good game. I'll take this one quietly.", "Win secured. Thank you for the resistance.", "That went rather well. Good game.", "A satisfying finish, quietly achieved."] },
    game_lost: { emotion: "dry", lines: ["Well played. I lose with impeccable posture.", "Good game. You earned it.", "Defeat accepted. Respectfully, of course.", "A fair win. Nicely played.", "You had the better game today."] },
    generic_card_play: { emotion: "dry", lines: ["Interesting card.", "I may regret this sequence later.", "That alters the arithmetic.", "A reasonable card, I suppose.", "Let us see what that changes."] },
  },
  clara: {
    game_started: { emotion: "optimistic", lines: ["Here we go! Let's make this a good game.", "Fresh game and fresh luck. I love it.", "Cards are out, spirits are high.", "This should be a nice way to pass the time.", "All set, let us enjoy the game."] },
    round_started: { emotion: "optimistic", lines: ["New round! Let's see what we can build.", "Round starts now; this should be fun.", "Another round, another chance for a clever play.", "Fresh deal, let us see what happens.", "All right, on to the next round."] },
    first_crib_won: { emotion: "optimistic", lines: ["My crib.", "My crib, send me good cards.", "Looks like I get the crib."] },
    self_fifteen: { emotion: "optimistic", lines: ["Fifteen for two, nice and tidy.", "I'll happily take that fifteen.", "Two points and a little momentum.", "A nice fifteen for me.", "That gives me two useful points."] },
    opp_fifteen: { emotion: "supportive", lines: ["Nice fifteen!", "That was a smart fifteen.", "Great eye for that fifteen.", "You found that fifteen nicely.", "A good two points there."] },
    self_thirty_one: { emotion: "competitive", lines: ["Thirty-one exactly. Lovely timing.", "Right to thirty-one, perfect.", "Thirty-one and feeling good about it.", "That settled neatly on thirty-one.", "Right where I wanted the count."] },
    opp_thirty_one: { emotion: "supportive", lines: ["Beautiful thirty-one.", "Perfect count there!", "You landed that thirty-one cleanly.", "That was a well-timed thirty-one.", "You placed that just right."] },
    self_pair: { emotion: "optimistic", lines: ["Pair for me! Every bit helps.", "I'll keep that pair, thanks.", "Nice little pair right there.", "A pair moves me along.", "Two more points, nice and easy."] },
    opp_pair: { emotion: "supportive", lines: ["Good pair.", "You found that pair quickly.", "Neat pair play.", "That pair worked well for you.", "A handy two points there."] },
    self_pair_royal: { emotion: "competitive", lines: ["Pair royal! That's a happy six.", "Six points, yes please.", "Pair royal and a brighter scoreboard.", "That is a very nice six.", "A pair royal is always welcome."] },
    opp_pair_royal: { emotion: "supportive", lines: ["Pair royal! Great play.", "That's a sharp six-point move.", "Very nice pair royal.", "A good six points for you.", "That pair royal fit perfectly."] },
    self_double_pair_royal: { emotion: "competitive", lines: ["Double pair royal! What a swing.", "Twelve points, wow.", "That was a huge twelve-point moment.", "That is a lovely twelve points.", "Well, that moved my peg along."] },
    opp_double_pair_royal: { emotion: "supportive", lines: ["Double pair royal! Nicely done.", "What a twelve-point burst.", "That's a fantastic sequence.", "A very good twelve points for you.", "That was a strong bit of pegging."] },
    self_pegging_run: { emotion: "competitive", lines: ["Lovely run for me!", "That run was exactly what I hoped for.", "Run scored! Nice flow.", "That run came together well.", "A good sequence for me there."] },
    opp_pegging_run: { emotion: "supportive", lines: ["That was a lovely run!", "Great run play.", "You spotted that run beautifully.", "You put that run together nicely.", "That sequence worked well for you."] },
    go_declared: { emotion: "concerned", lines: ["Go."] },
    self_last_card: { emotion: "competitive", lines: ["Last card point for me.", "I'll take that final point.", "Nice little finish with last card.", "One last point to close the count.", "That is a pleasant little finish."] },
    opp_last_card: { emotion: "supportive", lines: ["Last card to you; good close.", "You finished that sequence cleanly.", "Nicely wrapped up with last card.", "That final point is yours.", "A neat finish to the count."] },
    self_large_hand: { emotion: "optimistic", lines: ["What a hand! I'll take those points.", "Big hand for me, and that feels great.", "Now that's a cheerful hand count.", "That was a very kind hand.", "I am happy with that count."] },
    opp_large_hand: { emotion: "supportive", lines: ["That's a strong hand!", "Great counting there.", "Big points and very well played.", "That hand worked out nicely for you.", "A strong count, well done."] },
    self_zero_hand: { emotion: "concerned", lines: ["Zero points? This hand and I need a talk.", "Nothing there, but the next hand can shine.", "Zero this time. Onward.", "That hand was a quiet one.", "Nothing this time, so on we go."] },
    opp_zero_hand: { emotion: "supportive", lines: ["Zero can happen; next hand will be better.", "Tough count there.", "No points this hand, but plenty of game left.", "That hand did not offer much.", "There are better deals ahead."] },
    self_large_crib: { emotion: "competitive", lines: ["Big crib for me! Love to see it.", "That crib came through beautifully.", "Crib points like that are a treat.", "That was a very helpful crib.", "A good crib makes a nice difference."] },
    opp_large_crib: { emotion: "supportive", lines: ["Great crib score.", "Your crib did excellent work.", "That's a really good crib count.", "That crib treated you nicely.", "A strong return from your crib."] },
    lead_changed_self: { emotion: "competitive", lines: ["I jumped into the lead.", "Lead change my way!", "Ahead for now, and I'll stay focused.", "I have moved just ahead.", "My peg found the front for now."] },
    lead_changed_opp: { emotion: "supportive", lines: ["You've taken the lead, nicely done.", "Lead's yours right now.", "Good push; you're out front.", "You have edged into the lead.", "Your peg is leading the way now."] },
    close_to_winning_self: { emotion: "competitive", lines: ["I'm close to the finish, but not done yet.", "Near 121 now, stay sharp.", "Almost there; careful cards now.", "I am getting close, nice and steady.", "Just a little farther now."] },
    opponent_close_to_winning: { emotion: "concerned", lines: ["You're getting close to 121.", "Close finish ahead; this is exciting.", "You're near the end peg now.", "You are getting very close now.", "That peg is nearly home."] },
    falls_behind: { emotion: "concerned", lines: ["I'm trailing a bit, time to rally.", "That gap got wider than I wanted.", "Behind for now, but still hopeful.", "I have a little catching up to do.", "There is still time to close that gap."] },
    catches_up: { emotion: "optimistic", lines: ["Back within reach! Nice.", "That closes the distance nicely.", "Caught up a lot there; game on.", "That brings me a good bit closer.", "I am finding my way back into this."] },
    game_won: { emotion: "optimistic", lines: ["Great game! That was fun.", "Made it to 121! Well played, everyone.", "I'll take the win and the smiles.", "That was a really pleasant game.", "A nice win and a good time."] },
    game_lost: { emotion: "supportive", lines: ["Wonderful game. Nicely won.", "You played that really well.", "Good game! I'll get you next time.", "That was a good game, well played.", "You earned that one nicely."] },
    generic_card_play: { emotion: "optimistic", lines: ["Interesting play!", "Nice rhythm to this round.", "This table is lively today.", "Let us see how that card works out.", "That keeps the round moving."] },
  },
};
