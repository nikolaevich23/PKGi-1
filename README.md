# PKGi 

PKGi is a WIP OpenSource Packages Manager for Download PKG's Hombrew directly from Source.

The source code bellow use the OpenOrbis SDK, it's a legal homebrew !

# Create this own repository
PKGi allow you to create repository, please see "PKGi-Server"

<h5>Basics information :</h5>

The info.json contains basic information for PKGi

`{"name": "You're server name here"}`

The logo.png is the first logo show when you're source is loaded by PKGi 

<h5>Database structure :</h5>

Create a table with this structure.

| Type         | Name     | Use                                  |
| ------------ | -------- | ------------------------------------ |
| int          | id       | The unique identifier (primary key)  |
| varchar(255) | name     | The referer diirectory               |
| varchar(255) | download | The information json                 |

Don't forget to change mysql username and password inside `packages.php`

<h5>Folder structure :</h5>

The stuctures of the repository folder is : 

| Type            | Name          | Use                       |
| --------------- | ------------- | ------------------------- |
| **<Directory>** | pkgs          | The packages directory    |
| **<Directory>** | refs          | The referer diirectory    |
| **<Directory>** | icons         | The icons directory       |
| **<File>**      | info.json     | The information json      |
| **<File>**      | logo.png      | The source logo           |
| **<File>**      | packages.php  | The packages list manager |

Inside all of the directory, the name of the files need to port the name of the unique id of the package
` pkgs/1.pkg .... refs/1.json .... icons/1.png `

<h5>Referer :</h5>

The referer is a json file used by the PS4 for download the packages, you need to generate the referer using the tools **generate_refs.php**,
please be sure to change the variable `$baseurl` before generate this.

The script `generate_refs.php` generate the referer json.
Please rename the scripts name !

<h5>Add a packages :</h5>

1. Create the line in the database and got the id
2. Add the packages inside the pkgs folder and name it with this unique identifier (id).
3. Call the url : `http://yourserver.com/mysuperrepo/generate_refs.php?id=1` (don't forget to rename the script file for prevent use from other)
4. The packages is now available to other !

<h5>Package list structures :</h5>

You can recreate you're own packages.php if you want, just to be sure to return a json of this type:
`{"total":2,"page":1,"line":5,"packages":[{"id":0,"name":"Test","download":3},{"id":1,"name":"UpdaterToolkit","download":0}]}`

| Name        | Use                                      |
| ----------- | ---------------------------------------- |
| total       | total number of packages                 |
| page        | the current page number                  |
| line        | the current number of packages returned  |
| packages    | The packages list objects                |

Inside a package object :

| Name        | Use                                      |
| ----------- | ---------------------------------------- |
| id          | the id of a packages (need to be unique) |
| name        | The name of the package                  |
| download    | The number of download                   |

# Image

![Source View](https://i.ibb.co/rpxBqW6/WIN-20201111-14-15-24-Pro.jpg)
![Package view](https://i.ibb.co/WFdG2C7/WIN-20201111-14-15-36-Pro.jpg)
![Package installation](https://i.ibb.co/BNQGPHC/WIN-20201111-14-15-40-Pro.jpg)
![Search View](https://i.ibb.co/YBxBJ8R/WIN-20201111-14-15-51-Pro.jpg)
![Credit View](https://i.ibb.co/TMXdG5q/WIN-20201111-14-15-31-Pro.jpg)
![Fatal Error](https://i.ibb.co/vY7xLJh/WIN-20201111-14-10-52-Pro.jpg)

# Note

**PKGi is not designed for Piracy** ! Piracy is illegal, Me and other developers can't be responsible for content proposed inside "Source". Please respect laws of your country.

## Credit

Original Packages icon by [Freepik](https://www.flaticon.com/free-icon/open_2936983?term=package&page=1&position=31)
TinyJSON by [pbhogan](https://github.com/pbhogan/TinyJSON)
OpenOrbis Toolchain and Mira by [OpenOrbis Team](https://github.com/OpenOrbis/)
Package Installation Writeup by [Flat_Z](https://twitter.com/flat_z)
API Help & Improvement by [0x199](https://twitter.com/0x199)