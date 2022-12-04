import matplotlib.pyplot as plt
import json
import numpy as np
import typing
import time
import colorsys


class HSV:
    def __init__(self, h, s, v):
        self.h = h
        self.s = s
        self.v = v

    def __repr__(self):
        return f"HSV({self.h}, {self.s}, {self.v})"

    def to_rgb(self) -> typing.Tuple[int, int, int]:
        """Convert this HSV object to an RGB tuple

        Returns:
            typing.Tuple[int, int, int]: The RGB tuple
        """
        return tuple(round(i * 255) for i in colorsys.hsv_to_rgb(self.h/360, self.s/100, self.v/100))

    def copy(self) -> 'HSV':
        """Generate a copy of this HSV object

        Returns:
            HSV: A copy of this HSV object
        """
        return HSV(self.h, self.s, self.v)

    @staticmethod
    def from_rgb(r: int, g: int, b: int) -> 'HSV':
        """Convert an RGB tuple to an HSV object

        Args:
            r (int): The red value
            g (int): The green value
            b (int): The blue value

        Returns:
            HSV: The HSV object
        """
        h, s, v = colorsys.rgb_to_hsv(r/255, g/255, b/255)
        return HSV(h*360, s*100, v*100)

    @staticmethod
    def scale_to_hsv(h: int, s: int, v: int) -> 'HSV':
        """Scale given HSV values from 0-255 to 0-360

        Args:
            h (int): Hue
            s (int): Saturation
            v (int): Value

        Returns:
            HSV: Scaled HSV object
        """

        return HSV(h * 360 / 255, s * 100 / 255, v * 100 / 255)

    def scale_from_hsv(self) -> typing.Tuple[int, int, int]:
        """Scale given HSV object from 0-360 to 0-255

        Args:
            hsv (HSV): HSV object to scale

        Returns:
            Tuple[int, int, int]: Scaled HSV values
        """

        return (int(self.h * 255 / 360), int(self.s * 255 / 100), int(self.v * 255 / 100))

    @staticmethod
    def scale_rgb_to_1(rgb: typing.Tuple[int, int, int]) -> typing.Tuple[float, float, float]:
        """Scale RGB values from 0-255 to 0-1

        Args:
            rgb (Tuple[int, int, int]): RGB values

        Returns:
            Tuple[float, float, float]: Scaled RGB values
        """

        return tuple(i / 255 for i in rgb)


class Position:
    positions: dict[int, 'Position'] = {}

    def __init__(self, pos: int, coord: typing.Tuple[int, int, int]):
        self.pos = pos
        self.x, self.y, self.z = coord

        self.current_color = HSV(0, 0, 0)

        self.fade_from_color = None
        self.fade_to_color = None
        self.fade_time = -1
        self.fade_start_time = None

        self.positions[pos] = self

    def __repr__(self):
        return f"Position({self.pos}, ({self.x}, {self.y}, {self.z}), {self.current_color})"

    @classmethod
    def from_json(cls, json: dict) -> list['Position']:
        """Generate Position objects from a json object

        Args:
            json (dict): The json object
        """
        for pos, coord in json.items():
            cls(int(pos), coord)

    @classmethod
    def get(cls, pos: int) -> 'Position':
        """Get a position by its index on a string of LEDs

        Args:
            pos (int): Index of the LED

        Returns:
            Position: The position object
        """
        return cls.positions[pos]

    @classmethod
    def all(cls) -> dict[int, 'Position']:
        """Get all positions

        Returns:
            dict['Position']: All positions
        """
        return cls.positions

    @classmethod
    def in_area(cls, x1: int, y1: int, z1: int, x2: int, y2: int, z2: int, color: HSV) -> dict[int, 'Position']:
        """Get all LEDs in a given area

        Args:
            x1 (int): X coordinate of the first corner
            y1 (int): Y coordinate of the first corner
            z1 (int): Z coordinate of the first corner
            x2 (int): X coordinate of the second corner
            y2 (int): Y coordinate of the second corner
            z2 (int): Z coordinate of the second corner
            color (HSV): The color to set the LEDs to
        """
        positions = {}

        for pos in cls.positions.values():
            if pos.x >= x1 and pos.x <= x2 and pos.y >= y1 and pos.y <= y2 and pos.z >= z1 and pos.z <= z2:
                positions[pos.pos] = pos

        return positions

    @classmethod
    def max_x(cls) -> int:
        """Get the maximum x coordinate

        Returns:
            int: The maximum x coordinate
        """
        return max([pos.x for pos in cls.positions.values()])

    @classmethod
    def max_y(cls) -> int:
        """Get the maximum y coordinate

        Returns:
            int: The maximum y coordinate
        """
        return max([pos.y for pos in cls.positions.values()])

    @classmethod
    def max_z(cls) -> int:
        """Get the maximum z coordinate

        Returns:
            int: The maximum z coordinate
        """
        return max([pos.z for pos in cls.positions.values()])

    def _apply_fade(self, current_time: float):
        """Update the current color based on the fade

        Args:
            current_time (float): The current time
        """
        if self.fade_time == -1:
            return

        if current_time < self.fade_start_time:
            return

        if current_time > self.fade_start_time + self.fade_time:
            self.current_color = self.fade_to_color.copy()
            self.fade_time = -1
            return

        time_passed = current_time - self.fade_start_time
        # print(time_passed)
        progress = time_passed / self.fade_time

        self.current_color.h = self._blend(
            self.fade_from_color.h, self.fade_to_color.h, progress)
        self.current_color.s = self._blend(
            self.fade_from_color.s, self.fade_to_color.s, progress)
        self.current_color.v = self._blend(
            self.fade_from_color.v, self.fade_to_color.v, progress)

        fade_from_RGB = self.fade_from_color.to_rgb()
        fade_to_RGB = self.fade_to_color.to_rgb()

        current_fade = [0, 0, 0]

        current_fade[0] = self._blend(
            fade_from_RGB[0], fade_to_RGB[0], progress)
        current_fade[1] = self._blend(
            fade_from_RGB[1], fade_to_RGB[1], progress)
        current_fade[2] = self._blend(
            fade_from_RGB[2], fade_to_RGB[2], progress)

        self.current_color = HSV.from_rgb(*current_fade)

    def _blend(self, value1: int, value2: int, progress: float) -> int:
        """Blend between two values

        Args:
            value1 (int): First value
            value2 (int): Second value
            progress (float): The progress between the two values (0-1)

        Returns:
            int: The blended value
        """

        return int(value1 * (1 - progress) + value2 * progress)


class Task:
    """
    Represents an ldam task that can be executed by the lights.
    """

    def __init__(self, position: Position, color: HSV, delay: float = 0, fadeColor: HSV = HSV(0, 0, 0), fadeTime: float = -1):
        self.position = position
        self.color = color
        self.delay = delay
        self.fadeColor = fadeColor
        self.fadeTime = fadeTime

    def __repr__(self):
        return f"Task({self.position}, {self.color}, {self.delay}, {self.fadeColor}, {self.fadeTime})"

    def compile(self) -> str:
        """Compiles to an ldam string in the format: "pos,h,s,v,delay,fade_h,fade_s,fade_v,fadeTime"
        """
        return f"{self.position.pos},{','.join(str(i) for i in self.color.scale_from_hsv())},{self.delay},{','.join(str(i) for i in self.fadeColor.scale_from_hsv())},{self.fadeTime}"

    def copy(self) -> 'Task':
        """Returns a copy of the task
        """
        return Task(self.position, self.color, self.delay, self.fadeColor, self.fadeTime)

    @classmethod
    def from_ldam(cls, ldam: str) -> list['Task']:
        """Parse an ldam string into a list of Task objects

        Args:
            ldam (str): The ldam string

        Returns:
            list['Task']: A list of Task objects
        """

        ldam = ldam.split("|")
        tasks = [cls._parse_command(command) for command in ldam]

        return tasks

    def _parse_command(self, command: str) -> 'Task':
        """Takes a command string and returns a Task object

        Args:
            command (str): Command string

        Returns:
            Task: Task object
        """

        command = command.split(",")

        pos = int(command[0])
        color = HSV.scale_to_hsv(command[1], command[2], command[3])
        delay = int(command[4])

        try:
            fadeColor = HSV.scale_to_hsv(command[5], command[6], command[7])
            fadeTime = int(command[8])
        except IndexError:
            fadeColor = None
            fadeTime = None

        return Task(Position.get(pos), HSV(*color), delay, HSV(*fadeColor), fadeTime)


class ldamManager:
    """Reads and writes ldam files"""

    def __init__(self, max_tasks: int = 1600):
        self.max_tasks = max_tasks

    def read(self, path: str) -> str:
        """Reads an ldam file

        Args:
            path (str): The path to the ldam file

        Returns:
            str: The ldam string
        """
        with open(path, "r"):
            return f.read()

    def write(self, path: str, ldam: str):
        """Writes an ldam string to a file

        Args:
            path (str): The path to the ldam file
            ldam (str): The ldam string
        """
        with open(path, "w") as f:
            f.write(ldam)

    def format(self, tasks: list[Task]) -> str:
        """Formats a list of Task objects into an ldam string

        Args:
            tasks (list[Task]): The list of Task objects

        Returns:
            str: The ldam string
        """
        if len(tasks) > self.max_tasks:
            raise ValueError(f"Too many tasks! Max tasks: {self.max_tasks}")

        return "|".join([task.compile() for task in tasks])


class Visualize:
    def __init__(self, tasks: list[Task], positions: dict[int, Position]):
        self.tasks = tasks
        self.positions = positions

    def show(self, repeat=True):
        """Displays a simulation of the ldam command string

        Args:
            repeat (bool, optional): Whether to repeat the animation. Defaults to True.
        """

        values = self.positions.values()

        x = [p.x for p in values]
        y = [p.y for p in values]
        z = [p.z for p in values]

        fig = plt.figure()
        self.ax = fig.add_subplot(111, projection='3d')

        self.ax.scatter(x, z, y, color=(0, 0, 0), marker='o')

        self.ax.set_xlabel('X')
        self.ax.set_ylabel('Z')
        self.ax.set_zlabel('Y')

        # Set scale to 1:1:1
        max_range = np.array([x, y, z]).max()
        mid_x = (max(x) + min(x)) * 0.3
        mid_y = (max(y) + min(y)) * 0.3
        mid_z = (max(z) + min(z)) * 0.3
        self.ax.set_xlim(mid_x - max_range, mid_x + max_range)
        self.ax.set_ylim(mid_z - max_range, mid_z + max_range)
        self.ax.set_zlim(mid_y - max_range, mid_y + max_range)

        self.start_time = time.time() * 1000
        max_time = max([task.delay for task in self.tasks])

        temp_tasks = [task.copy() for task in self.tasks]

        loops = 0

        while True:
            time_passed = time.time() * 1000 - self.start_time

            self.ax.clear()

            # Get min delay
            min_delay = min(task.delay for task in temp_tasks)

            # Get all tasks with delay less than time passed
            tasks = [task for task in temp_tasks if task.delay <= time_passed]

            for task in tasks:
                if task.delay <= time_passed:
                    temp_tasks.remove(task)

                    task.position.current_color = task.color.copy()

                    if task.fadeTime != -1:
                        task.position.fade_from_color = task.color.copy()
                        task.position.fade_to_color = task.fadeColor.copy()
                        task.position.fade_time = task.fadeTime
                        task.position.fade_start_time = time_passed

            # print(f"{int(time_passed)}:", len(temp_tasks))
            if len(temp_tasks) == 0:
                if repeat:
                    loops += 1
                    temp_tasks = [task.copy() for task in self.tasks]
                    for task in temp_tasks:
                        task.delay += max_time * loops
                else:
                    break

            for pos in self.positions.values():
                pos._apply_fade(time_passed)

            x = [p.x for p in self.positions.values()]
            y = [p.y for p in self.positions.values()]
            z = [p.z for p in self.positions.values()]

            c = [HSV.scale_rgb_to_1(p.current_color.to_rgb()) for p in self.positions.values()]

            self.ax.scatter(x, z, y, color=c, marker='o')

            max_range = np.array([x, y, z]).max()
            mid_x = (max(x) + min(x)) * 0.3
            mid_y = (max(y) + min(y)) * 0.3
            mid_z = (max(z) + min(z)) * 0.3
            self.ax.set_xlim(mid_x - max_range, mid_x + max_range)
            self.ax.set_ylim(mid_z - max_range, mid_z + max_range)
            self.ax.set_zlim(mid_y - max_range, mid_y + max_range)

            plt.pause(0.010)

            if not plt.fignum_exists(fig.number):
                break


if __name__ == "__main__":
    with open('positions3D.json', 'r') as f:
        positions = json.load(f)

    Position.from_json(positions)

    tasks: Task = []

    for position in Position.all().values():
        tasks.append(Task(position, HSV(190, 100, 100),
                     (Position.max_y() - position.y) * 5, HSV(0, 100, 100), 1000))
        tasks.append(Task(position, HSV(0, 100, 100),
                     (Position.max_y() - position.y) * 5 + 1000, HSV(0, 0, 0), 1000))

    tasks.append(Task(Position.get(49), HSV(0, 0, 0), Position.get(49).y * 5 + 1500))

    ldam = ldamManager()
    ldam.write("test.ldam", ldam.format(tasks))

    vis = Visualize(tasks, Position.all())
    vis.show()
