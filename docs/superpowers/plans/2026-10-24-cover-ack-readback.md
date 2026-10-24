# Cover: independent command acceptance and background status

Owner approved the documented flow in the conversation on 2026-10-24.
Scope: Cover controls, including native controls, groups/scenes and urgent STOP.
Keep the approved layout, protocol, identity/auth checks and sole worker.

1. Reproduce the bug with real HTTP fixtures and the real fleet: a null ACK
   must complete the command without waiting for a later GET. ACK must not
   fabricate a fresh sample, position, movement or target convergence.
2. End the Cover HTTP command at the validated ACK. Deliver the normal worker
   result immediately; schedule a separate read-only fleet job. Keep uncertain
   writes uncertain and never replay POST. Retain actor guards before the write.
3. Keep the command result separate from telemetry. Allow one background
   transport-read retry after ACK without flashing an error or extending sample
   freshness. Persistent read failures, malformed data, auth/identity errors and
   device safety errors remain visible. STOP uses the same handoff semantics.
4. Show acceptance/waiting separately from sending. Preserve real artwork and
   the bottom STOP during this interval; animate only received motion/position.
   Verify the changed LVGL view in three skins and PL/EN/DE.
5. Run focused platform, worker/actor and UI regressions, then one board build
   with relevant memory checks. Review the final diff. No release or device
   flash is part of this change; physical latency/transport acceptance is open.

Review focus: lost ACK versus lost GET; no synthetic telemetry/freshness;
urgent STOP superseding an outer move; stale IDs/changed endpoints; bounded
retry and fail-closed auth/identity/safety errors.

References: official Shelly Gen2+ Cover and RPC Channels documentation.
https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Cover/
https://shelly-api-docs.shelly.cloud/gen2/General/RPCChannels/
