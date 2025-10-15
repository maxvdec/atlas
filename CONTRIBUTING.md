# Contributing

Hello and thanks to take the time to contribute to this project! We want to keep the project organized for everyone, so here's a guide
on how to contribute and make the best to the repo!

Also, check the [Code Of Conduct](CODE_OF_CONDUCT.md) for more information on how to respect and take care of the community!

## How to make Atlas a better project

One of the first steps of contribution is helping to the developers to actually know what is wrong or what needs to be changed with the project:

- For problems, bugs, minor changes, or structured changes create **an Issue** here in GitHub. Pair it with a tag, and follow the proposed instructions in the template.

- For proposals, create **a Discussion**, we check them so we can hear about what needs to be changed. From there, we'll create the corresponding issues for developers to know what has to be done

## What do we do?

We also have a **Roadmap** where we list the **core, last-longing features** we take a lot time to implement, so you can see in what we're working in [README.md](README.md).

## How to structure a branch

We assume you know how to fork a repo and get started with modifying Atlas. Besides that, we have a strong policy about naming branches, so we can have our repo structured:

* Feature branches (the ones that implement new features) are prefixed with `feature/`
* Fix branches (from minor to big fixes) are prefixed with `fix/`
* Program branches (reorganization of files, optimizations, things that are not seen by the user) are prefixed with `program/`
* Repo branches (changing files for the repo structure, GitHub actions...) are prefixed with `repo/`
* Documentation branches (adding documentation) are prefixed with `docs/`
* Long-living branches (releases, etc...) aren't prefixed, these are rare, but each library has one

After the prefix, put a short description. If you are working for a specific library, add the library prefix. Here's a list of them:
* `bezel` is for **Bezel**
* `finewave` is for **Finewave**

For example, a branch fixing a bug in the rendering of Bezel, would be: `fix/bezel-rendering`. Thanks for keeping the repo clean and organized.

If your change has no global description (for instance, fixing general linting issues), put the date after it in European Style: `fix/27-07-2025`.

## How to get your Pull Request to be accepted
* Make sure all the tests are passing, use the commands in the `.github/workflows` to test the result
* Make sure to document every public part of the code, so other developers can understand
* In the Pull Request body, explain the changes clearly

## How to get inspiration

You can check the issues every now and then. If you can, try checking the ones with the **needed** tag, meaning they are needed by the community.

## Have fun

Try messing up Atlas, and have fun.
