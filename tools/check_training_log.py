"""Check a Training run captured with logfile 2, developer 1, g_debugMove 1 and cg_debugAnim 1."""
import argparse
import re
from pathlib import Path


RUNNERS = {141, 220, 221, 222, 223, 224, 225}
TRAVERSAL_CLIPS = {425, 428, 429, 430, 431}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check(log):
    first = re.search(r"^z:\s*got1carbine\s*$", log, re.MULTILINE)
    second = re.search(r"^z:\s*got2carbine\s*$", log, re.MULTILINE)
    require(first and second and first.end() < second.start(), "Missing or out-of-order M1 pickups.")
    require(re.search(r"\[voice start\][^\n]*alias=training_moody_carbine03\b", log[second.end():]),
            "Moody's follow-up after the second M1 was not recorded.")
    course = log[:first.start()]
    require(not re.search(r"clientCommand:.* : (?:setviewpos|noclip)\b", course),
            "The course used a movement override.")
    traversal = re.findall(
        r"\[570 traversal wire\] tick=\d+ owner=(\d+) assignment=\d+ clip=(\d+) "
        r"phase=[.\d]+ version=(\d+) frame=(\d+) flags=(\d+)", course)
    clips = {}
    for owner, clip, version, frame, flags in traversal:
        require(version == "25" and frame == clip and flags == "3", "Invalid traversal packet.")
        clips.setdefault(int(owner), set()).add(int(clip))
    require(set(clips) == RUNNERS and all(clips[runner] == TRAVERSAL_CLIPS for runner in RUNNERS),
            "Each of the seven soldiers must publish the fence and all four ladder clips.")

    wire = re.findall(
        r"\[572 Foley walk wire\] tick=(\d+) owner=(\d+) assignment=(\d+) moving=(\d+) "
        r"blocked=(\d+) arrived=(\d+) active=(-?\d+) phase=([.\d]+) distance=([.\d]+)", course)
    require(len({row[1] for row in wire}) == 1, "Missing or ambiguous Foley movement samples.")
    owner = wire[0][1]
    moving = []
    for tick, _, assignment, motion, blocked, arrived, active, phase, distance in wire:
        walking = motion == "1" and blocked == arrived == "0"
        if not walking:
            require(active == "0", "Foley continued walking after stopping.")
        if walking and assignment == "2":
            require(active == "1", "Foley moved along the wire without an active walk.")
            moving.append(int(tick))
        if active == "1":
            delta = abs(float(phase) - (float(distance) / 63.4768) % 1)
            require(min(delta, 1 - delta) < 0.000004, "Walk phase drifted from committed distance.")
    require(len(moving) >= 5, "Insufficient Foley movement samples at the wire.")

    rendered = re.findall(
        r"\[572 Foley walk render\] tick=(\d+) owner=(\d+) active=(-?\d+) phase=[.\d]+ "
        r"left=([-\d.,]+) right=([-\d.,]+) head=[-\d.,]+ accepted=1", course)
    feet = [(left, right) for tick, who, active, left, right in rendered
            if who == owner and active == "1" and min(moving) <= int(tick) <= max(moving)]
    require(len(feet) >= 5, "Insufficient rendered feet samples during movement.")
    for side in zip(*feet):
        points = [list(map(float, point.split(","))) for point in side]
        require(all(len(point) == 3 for point in points) and
                any(max(axis) - min(axis) > 5 for axis in zip(*points)),
                "Foley's rendered foot did not move through the expected span.")
    require(any(who == owner and active == "0" and int(tick) > max(moving)
                for tick, who, active, _, _ in rendered), "Missing rendered stop after the wire.")
    return f"7 soldiers, {len(traversal)} traversal packets, {len(feet)} walking poses, both M1 pickups."


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="Path to profile/main/rtcwconsole.log")
    args = parser.parse_args()
    try:
        print(check(args.log.read_text(encoding="latin1")))
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")
