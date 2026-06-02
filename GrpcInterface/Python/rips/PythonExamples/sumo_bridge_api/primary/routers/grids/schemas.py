from pydantic import BaseModel


class GridInfo(BaseModel):
    name: str
    realizations: list[int]
