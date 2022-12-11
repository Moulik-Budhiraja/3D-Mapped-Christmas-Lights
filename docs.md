## class `HSV`

    """
    Represents a HSV color object
    """

### def h(self) -> int:

        """Hue

        Returns:
            int: Hue
        """

### def h(self, value: int):

        """Hue

        Args:
            value (int): Hue
        """

### def s(self) -> int:

        """Saturation

        Returns:
            int: Saturation
        """

### def s(self, value: int):

        """Saturation

        Args:
            value (int): Saturation
        """

### def v(self) -> int:

        """Value

        Returns:
            int: Value
        """

### def v(self, value: int):

        """Value

        Args:
            value (int): Value
        """

### def copy(self) -> 'HSV':

        """Generate a copy of this HSV object

        Returns:
            HSV: A copy of this HSV object
        """

### def from_rgb(r: int, g: int, b: int) -> 'HSV':

        """Convert an RGB tuple to an HSV object

        Args:
            r (int): The red value
            g (int): The green value
            b (int): The blue value

        Returns:
            HSV: The HSV object
        """

## class `Position`

    """
    Represents a position on a string of LEDs
    """

### def from_json(cls, json: dict) -> list['Position']:

        """Generate Position objects from a json object

        Args:
            json (dict): The json object
        """

### def get(cls, pos: int) -> 'Position':

        """Get a position by its index on a string of LEDs

        Args:
            pos (int): Index of the LED

        Returns:
            Position: The position object
        """

### def all(cls) -> dict[int, 'Position']:

        """Get all positions

        Returns:
            dict['Position']: All positions
        """

### def in_area(cls, x1: int, y1: int, z1: int, x2: int, y2: int, z2: int) -> dict[int, 'Position']:

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

### def max_x(cls) -> int:

        """Get the maximum x coordinate

        Returns:
            int: The maximum x coordinate
        """

### def max_y(cls) -> int:

        """Get the maximum y coordinate

        Returns:
            int: The maximum y coordinate
        """

### def max_z(cls) -> int:

        """Get the maximum z coordinate

        Returns:
            int: The maximum z coordinate
        """

### def \_apply_fade(self, current_time: float):

        """Update the current color based on the fade

        Args:
            current_time (float): The current time
        """

## class `Task`

    """
    Represents an ldam task that can be executed by the lights.
    """

### def copy(self) -> 'Task':

        """Returns a copy of the task
        """

### def from_ldam(cls, ldam: str) -> list['Task']:

        """Parse an ldam string into a list of Task objects

        Args:
            ldam (str): The ldam string

        Returns:
            list['Task']: A list of Task objects
        """

### def \_parse_command(self, command: str) -> 'Task':

        """Takes a command string and returns a Task object

        Args:
            command (str): Command string

        Returns:
            Task: Task object
        """

## class `ldamManager`

    """Reads and writes ldam files"""

### def read(self, path: str) -> str:

        """Reads an ldam file

        Args:
            path (str): The path to the ldam file

        Returns:
            str: The ldam string
        """

### def write(self, path: str, ldam: str):

        """Writes an ldam string to a file

        Args:
            path (str): The path to the ldam file
            ldam (str): The ldam string
        """

### def format(self, tasks: list[Task]) -> str:

        """Formats a list of Task objects into an ldam string

        Args:
            tasks (list[Task]): The list of Task objects

        Returns:
            str: The ldam string
        """

### def upload(self, host: str, ldam: str, add=False):

        """Uploads a list of Task objects to an ldam server

        Args:
            ldam (str): The ldam string
            host (str): The host of the ldam server
        """

## class `Visualize`

    """
    Visualizes an ldam string
    """

### def show(self, repeat=True):

            """Displays a simulation of the ldam command string

            Args:
                repeat (bool, optional): Whether to repeat the animation. Defaults to True.
            """
