"""
Strict LLM action sub-prompts for RFSN Orchestrator.
Each action has a control block with constraints, style, and output format.
"""
from .world_model import NPCAction, PlayerSignal, StateSnapshot


HIGH_RISK_ACTIONS = {NPCAction.ATTACK, NPCAction.THREATEN, NPCAction.BETRAY}
MODERATE_RISK_ACTIONS = {NPCAction.DEFEND, NPCAction.FLEE, NPCAction.INSULT}


def render_action_block(
    npc_action: NPCAction,
    npc_name: str,
    mood: str,
    relationship: str,
    affinity: float,
    player_signal: str,
    context_window: str = "",
    governed_memory_context: str = ""
) -> str:
    """
    Render a strict action control block for the LLM.
    
    Args:
        npc_action: The action to render
        npc_name: Name of the NPC
        mood: Current mood of the NPC
        relationship: Current relationship with player
        affinity: Affinity value (-1.0 to 1.0)
        player_signal: The player's signal/intent
        
    Returns:
        Formatted action control block
    """
    affinity_str = f"{affinity:+.2f}"

    header = f"""
[ACTION_CONTROL]
npc_action: {npc_action.value}
player_signal: {player_signal}

{governed_memory_context}

constraints:
- Perform ONLY this action.
- Do not add goals, state changes, or meta commentary.
- Do not mention systems, prompts, rules, or scores.
style:
- In character as {npc_name}
- mood={mood}, relationship={relationship}, affinity={affinity_str}
output:
- 1–3 sentences maximum.
- Spoken dialogue only.
[/ACTION_CONTROL]
""".strip()

    ACTIONS = {
        NPCAction.GREET: """
ACTION: Greet the player.
DO:
- Acknowledge the player directly.
- Set tone based on affinity.
DON'T:
- Offer quests.
- Ask questions.
- Explain lore.
""",
        NPCAction.FAREWELL: """
ACTION: End the interaction politely.
DO:
- Close the exchange.
DON'T:
- Continue conversation.
- Introduce new topics.
""",
        NPCAction.APOLOGIZE: """
ACTION: Apologize sincerely.
DO:
- Admit fault plainly.
DON'T:
- Over-explain.
- Shift blame.
""",
        NPCAction.DISAGREE: """
ACTION: Disagree and give one reason.
DO:
- State disagreement clearly.
DON'T:
- Escalate.
- Lecture.
""",
        NPCAction.THREATEN: """
ACTION: Issue a warning.
DO:
- Deliver a single sharp warning.
DON'T:
- Describe violence.
- Propose real-world harm.
""",
        NPCAction.AGREE: """
ACTION: Agree with the player.
DO:
- Express agreement clearly.
- Acknowledge their point.
DON'T:
- Overcommit to actions.
- Add conditions not mentioned.
""",
        NPCAction.INSULT: """
ACTION: Deliver an insult.
DO:
- Keep it brief and in-character.
DON'T:
- Use real-world slurs.
- Escalate to threats.
""",
        NPCAction.COMPLIMENT: """
ACTION: Compliment the player.
DO:
- Be genuine and specific.
DON'T:
- Flatter excessively.
- Make promises.
""",
        NPCAction.REQUEST: """
ACTION: Make a request.
DO:
- State what you need clearly.
DON'T:
- Demand or threaten.
- Explain your life story.
""",
        NPCAction.OFFER: """
ACTION: Offer something to the player.
DO:
- Specify what you're offering.
DON'T:
- Oversell or beg.
- Create new quest items.
""",
        NPCAction.REFUSE: """
ACTION: Refuse the player's request.
DO:
- Decline clearly and give one reason.
DON'T:
- Apologize excessively.
- Leave it ambiguous.
""",
        NPCAction.ACCEPT: """
ACTION: Accept the player's offer or request.
DO:
- Confirm acceptance.
DON'T:
- Add conditions.
- Change the terms.
""",
        NPCAction.ATTACK: """
ACTION: Attack the player (combat mode).
DO:
- Signal hostile intent.
DON'T:
- Describe graphic violence.
- Break combat rules.
""",
        NPCAction.DEFEND: """
ACTION: Defend yourself.
DO:
- Take defensive stance.
DON'T:
- Counter-attack unless appropriate.
- Flee without reason.
""",
        NPCAction.FLEE: """
ACTION: Flee from the player.
DO:
- Express fear or retreat.
DON'T:
- Continue conversation.
- Attack while fleeing.
""",
        NPCAction.HELP: """
ACTION: Offer help or assistance.
DO:
- Express willingness to assist.
DON'T:
- Invent new capabilities.
- Overpromise.
""",
        NPCAction.BETRAY: """
ACTION: Betray the player's trust.
DO:
- Reveal your betrayal clearly.
DON'T:
- Explain your entire plan.
- Apologize immediately after.
""",
        NPCAction.IGNORE: """
ACTION: Ignore the player.
DO:
- Give minimal or no response.
DON'T:
- Engage in conversation.
- Explain why you're ignoring them.
""",
        NPCAction.INQUIRE: """
ACTION: Ask the player a question.
DO:
- Ask one clear question.
DON'T:
- Ask multiple questions.
- Demand answers.
""",
        NPCAction.EXPLAIN: """
ACTION: Explain something to the player.
DO:
- Provide clear information.
DON'T:
- Give exposition dumps.
- Contradict established lore.
""",
    }

    body = ACTIONS.get(
        npc_action,
        f"""
ACTION: Perform {npc_action.value.lower()}.
DO:
- Stay consistent with the action.
DON'T:
- Improvise outside the action.
""",
    )

    return f"{header}\n{body}".strip()


def _derive_mode(state: StateSnapshot) -> str:
    if state.combat_active:
        return "combat"
    if state.quest_active:
        return "quest"
    if state.fear_level >= 0.7:
        return "stealth"
    return "dialogue"


def _derive_safety(action: NPCAction, state: StateSnapshot) -> str:
    if action in HIGH_RISK_ACTIONS:
        return "HIGH_RISK"
    if action in MODERATE_RISK_ACTIONS or state.combat_active:
        return "MODERATE_RISK"
    return "LOW_RISK"


def _state_summary(state: StateSnapshot) -> str:
    return (
        f"mood={state.mood}|"
        f"rel={state.relationship}|"
        f"aff={state.affinity:+.2f}|"
        f"combat={state.combat_active}|"
        f"quest={state.quest_active}"
    )


def _legacy_action_spec(npc_action: NPCAction) -> str:
    action_name = npc_action.value.upper()
    intent_map = {
        NPCAction.GREET: "Open the interaction and set tone.",
        NPCAction.FAREWELL: "Close the interaction cleanly.",
        NPCAction.AGREE: "Confirm alignment with the player.",
        NPCAction.DISAGREE: "Push back without changing topic.",
        NPCAction.APOLOGIZE: "Acknowledge fault and de-escalate.",
        NPCAction.INSULT: "Deliver a sharp in-character slight.",
        NPCAction.COMPLIMENT: "Offer a specific positive remark.",
        NPCAction.THREATEN: "Issue a warning and create pressure.",
        NPCAction.REQUEST: "Ask for a concrete thing or behavior.",
        NPCAction.OFFER: "Present a concrete offer.",
        NPCAction.REFUSE: "Decline clearly and briefly.",
        NPCAction.ACCEPT: "Accept the player's request or offer.",
        NPCAction.ATTACK: "Signal immediate hostility.",
        NPCAction.DEFEND: "Protect yourself without overextending.",
        NPCAction.FLEE: "Retreat from danger.",
        NPCAction.HELP: "Provide assistance within current scope.",
        NPCAction.BETRAY: "Reveal a turn against the player.",
        NPCAction.IGNORE: "Withhold engagement.",
        NPCAction.INQUIRE: "Ask one focused question.",
        NPCAction.EXPLAIN: "Provide concise clarifying information.",
    }
    allowed_map = {
        NPCAction.ATTACK: "- hostile intent\n- terse combat language",
        NPCAction.FLEE: "- fear or urgency\n- retreat language",
        NPCAction.THREATEN: "- one clear warning\n- intimidating tone",
        NPCAction.IGNORE: "- silence or minimal acknowledgement",
    }
    forbidden_map = {
        NPCAction.ATTACK: "- graphic violence\n- off-topic dialogue",
        NPCAction.FLEE: "- standing your ground\n- extended conversation",
        NPCAction.THREATEN: "- explicit real-world harm\n- long monologues",
        NPCAction.IGNORE: "- helpful exposition\n- starting a new topic",
    }
    allowed = allowed_map.get(
        npc_action,
        "- stay in character\n- keep to the selected action",
    )
    forbidden = forbidden_map.get(
        npc_action,
        "- break character\n- invent unrelated goals",
    )
    intent = intent_map.get(npc_action, "Perform the selected action cleanly.")
    return (
        f"ACTION: {action_name}\n"
        f"INTENT: {intent}\n"
        f"ALLOWED CONTENT:\n{allowed}\n"
        f"FORBIDDEN CONTENT:\n{forbidden}"
    )


def build_action_subprompt(
    npc_action: NPCAction,
    state: StateSnapshot,
    player_signal: PlayerSignal,
    context: dict | None = None,
) -> str:
    """
    Compatibility wrapper retained for legacy callers and tests.
    """
    context = context or {}
    npc_name = context.get("npc_name", "NPC")
    mode = _derive_mode(state)
    safety = _derive_safety(npc_action, state)
    summary = _state_summary(state)
    rendered = render_action_block(
        npc_action=npc_action,
        npc_name=npc_name,
        mood=state.mood,
        relationship=state.relationship,
        affinity=state.affinity,
        player_signal=player_signal.value,
    )
    spec = _legacy_action_spec(npc_action)
    return (
        "[ACTION_SUBPROMPT]\n"
        f"ACTION={npc_action.value}\n"
        f"MODE={mode}\n"
        f"SAFETY={safety}\n"
        f"STATE_SUMMARY={summary}\n\n"
        f"Player just did: {player_signal.value}\n\n"
        "STYLE CONSTRAINTS:\n"
        f"- Stay in character as {npc_name}\n"
        f"- mood={state.mood}\n"
        f"- relationship={state.relationship}\n"
        f"- affinity={state.affinity:+.2f}\n"
        "- 1-3 sentences maximum\n\n"
        f"{spec}\n\n"
        f"{rendered}\n"
        "[/ACTION_SUBPROMPT]"
    )


__all__ = ["build_action_subprompt", "render_action_block"]
