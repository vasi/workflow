This is a re-implementation of the Workflow module, using the Field API instead of the Form API.

ONLY USE THIS MODULE IF: 
- you are happy with a simple field, and the features the Workflow API provides
  (not all persons may choose from all possible values at all moments.)
- you want to test and help developing this submodule.

The current version supports: 
- the default Workflow API. 
- Workflow Admin UI, which manages CRUD for Workflows, States and Transitions.
- Workflow Access, since this works via Workflow API.

The current version provides: 
- adding a Workflow Field on an Entity type (Node type), or a Node Comment;
- usage of the default widgets from the Options module (select list, radio buttons);
- usage of the default formatter from the List module (just showing the description of the current value);
- changing the 'Workflow state' value on a Node Edit page.
- changing the 'Workflow state' value via a Node's Comment.

The current version DOES NOT provide: 
- the usage of the usual Workflow Form, which contains also a Comment text area and Scheduling options.
- support for other submodules from the Workflow module. (At least, this is not tested.)
